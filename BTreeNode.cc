#include "BTreeNode.h"
#include <iostream>
using namespace std;
//const string BPTREE = "B+Tree";
const int LEAF_ENTRY_SIZE = 12;
const int LEAF_NUM_ENTRIES = 84; //(PAGE_SIZE-4)/LEAF_ENTRY_SIZE;
const int NON_LEAF_ENTRY_SIZE = 8;
const int NON_LEAF_ENTRIES= 127; // PAGE_SIZE/NON_LEAF_ENTRY_SIZE - 1

/*Helper Functions*/

int BTLeafNode::getKey(int location){
	return (*((int*)(&buffer[location+8])));
}
int BTLeafNode::getRecord(int location, RecordId& rid){
	rid = *((RecordId*)(&buffer[location]));
	return 0;
}
int BTLeafNode::insertKey(int x, int location){
	memcpy((void*)(&buffer[location+8]), (void*)&(x), sizeof(int)); 
	return 0;
}
int BTLeafNode::insertRecordId(const RecordId rid, int location){
	memcpy((void*)(&buffer[location]), (void*)&rid, sizeof(RecordId));
	return 0;
}
int BTLeafNode::incrementKeyCount(){
	cout<<"      keyCount is incremented!!!"<<endl;
	keyCount++;
	memcpy((void*)(&buffer[PageFile::PAGE_SIZE-8]), (void*)&(keyCount), sizeof(int)); 
	return 0;
}
bool BTLeafNode::isNodeFull(){
	//cout<<keyCount<<" >= "<<LEAF_NUM_ENTRIES-1<<endl;
	return (keyCount >= (LEAF_NUM_ENTRIES-1));
}
RC BTLeafNode::getNodeId(){
	return leafNodePid;
}
/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, PageFile& pf){ 
	RC readError = 0;
	RC openError = 0;
	//openError = pf.open(BPTREE,'r');
	if(openError != 0)
		return openError;
	//cout<<"3) keycount is"<<keyCount<<endl;
	//cout<<"buffer is "<<*((int*)(&buffer[PageFile::PAGE_SIZE-8]))<<endl;
	cout<<"IN Read"<<endl;
	//cout<<"  pid is "<<pid<<endl;
	readError = pf.read(pid, (void*)buffer);
	//cout<<"buffer is "<<*((int*)(&buffer[PageFile::PAGE_SIZE-8]))<<endl;
	//cout<<"3) keycount is"<<keyCount<<endl;
	//cout<<*((int*)(&buffer[PageFile::PAGE_SIZE-8]))<<endl;
	//cout<<*((int*)(&buffer[8]))<<endl;
	memcpy((int*)&(keyCount), (int*)(&buffer[PageFile::PAGE_SIZE-8]), sizeof(int)); 
	cout<<"KeyCount in Read) keyCount is"<<keyCount<<endl;	
	leafNodePid = pid;
	//pf.close();
	return readError;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf){ 
	RC writeError = 0;
	RC openError = 0;
	//openError = pf.open(BPTREE,'w');
	if(openError != 0)
		return openError;
	cout<<"KeyCount before Write) keycCount is"<<keyCount<<endl;
	//cout<<*((int*)(&buffer[PageFile::PAGE_SIZE-8]))<<endl;
	//cout<<*((int*)(&buffer[8]))<<endl;
	memcpy((int*)(&buffer[PageFile::PAGE_SIZE-8]),(int*)&(keyCount), sizeof(int));
	//cout<<"2) keycount is"<<keyCount<<endl;
	//cout<<"buffer is "<<*((int*)(&buffer[PageFile::PAGE_SIZE-8]))<<endl;
	//cout<<"pid is "<<pid<<endl;
	writeError = pf.write(pid, (void*)(buffer));
	//cout<<"buffer is "<<*((int*)(&buffer[PageFile::PAGE_SIZE-8]))<<endl;	
	//pf.close();
	leafNodePid = pid;
	return writeError; 
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount(){ 
	return keyCount;
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid){ 
	RC insertError = RC_INVALID_ATTRIBUTE;
	int eidCompare,eidCount, keyCompare;
	eidCount = getKeyCount();
	if(eidCount == LEAF_NUM_ENTRIES){
		return insertError;
	}
	int firstByte, lastByte;
	insertError = locate(key, eidCompare);
	if(insertError != 0 || eidCount == 0){
		lastByte = eidCount*12;
		insertRecordId(rid, lastByte);
		insertKey(key, lastByte);
		cout<<"      keyCount is incremented!!!"<<endl;
		keyCount++;
	}else{
		keyCompare = getKey(eidCompare);
		if(keyCompare == key){
			return insertError;
		}
		firstByte = eidCompare*12;
		lastByte = eidCount*12-1;
		int copySize = lastByte - firstByte;
		char* tempBuffer = new char[copySize];
		memcpy((void*)tempBuffer, (void*)(&buffer[firstByte]), copySize);
		insertRecordId(rid, firstByte);
		insertKey(key, firstByte);
		memcpy((void*)(&buffer[firstByte+LEAF_ENTRY_SIZE]), (void*)tempBuffer, copySize);
		cout<<"      keyCount is incremented!!!"<<endl;		
		keyCount++;
	}
	return 0; 
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, BTLeafNode& sibling, int& siblingKey){
	insert(key, rid);
	int moveAmnt;
	if(keyCount%2 == 0){
		moveAmnt = keyCount/2;
	}else{
		moveAmnt = (keyCount-1)/2;
	}
	int tempKey;
	RecordId tempRid;
	siblingKey = getKey(moveAmnt*12);
	for(int i=moveAmnt; i<keyCount; i++){
		tempKey = getKey(i*12);
		getRecord(i*12, tempRid);
		sibling.insert(tempKey,tempRid);
	}
	keyCount = moveAmnt;
	return 0;
}

/*
 * Find the entry whose key value is larger than or equal to searchKey
 * and output the eid (entry number) whose key value >= searchKey.
 * Remeber that all keys inside a B+tree node should be kept sorted.
 * @param searchKey[IN] the key to search for
 * @param eid[OUT] the entry number that contains a key larger than or equalty to searchKey
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::locate(int searchKey, int& eid){
	int count = getKeyCount();
	int locateError = -1012;
	for(int i=0; i<count; i++){
		if(searchKey <= getKey(i*12)){
			eid = i;
			locateError = 0;
			break;
		}
	}
	return locateError; 
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid){ 
	int loc = eid*LEAF_ENTRY_SIZE;
	if(eid>getKeyCount())
		return RC_NO_SUCH_RECORD;
	key = getKey(loc);
	getRecord(loc, rid);
	return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr(){ 
	return (*((PageId*)(&buffer[PageFile::PAGE_SIZE-4])));
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid){
	(*((PageId*)(&buffer[PageFile::PAGE_SIZE-4]))) = pid;
	return 0;
}
/*****************************************************************************************************************************************************************************************************************************************************************************************************************************
END OF LEAF NODE -------- START NON LEAF NODE
**************************************************************************************************************************************************************/

/*Helper Functions*/
int BTNonLeafNode::getKey(int location){
	return (*((int*)(&buffer[location+4])));
}
int BTNonLeafNode::getPid(int location){
	return *((int*)(&buffer[location]));
}
int BTNonLeafNode::getLastPid(){
	return *((int*)(&buffer[PageFile::PAGE_SIZE]));
}
int BTNonLeafNode::insertKey(int x, int location){
	memcpy((void*)(&buffer[location+4]), (void*)&(x), sizeof(int)); 
	return 0;
}
int BTNonLeafNode::insertPid(const int pid, int location){
	memcpy((void*)(&buffer[location]), (void*)&pid, sizeof(int));
	return 0;
}
int BTNonLeafNode::insertLastPid(const int pid){
	memcpy((void*)(&buffer[PageFile::PAGE_SIZE-4]), (void*)&pid, sizeof(int));
	return 0;
}
int BTNonLeafNode::incrementKeyCount(){
	keyCount++;
	memcpy((void*)(&buffer[PageFile::PAGE_SIZE-8]), (void*)&(keyCount), sizeof(int)); 
	return 0;
}
bool BTNonLeafNode::isNodeFull(){
	return (keyCount >= (LEAF_NUM_ENTRIES-1));
}
RC BTNonLeafNode::getNodeId(){
	return nonLeafNodePid;
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, PageFile& pf){
	RC readError = 0;
	RC openError = 0;
	//openError = pf.open(BPTREE,'r');
	if(openError != 0)
		return openError;
	readError = pf.read(pid, (void*)buffer);
	memcpy((void*)&(keyCount), (void*)(&buffer[PageFile::PAGE_SIZE-8]), sizeof(int)); 
	//pf.close();
	nonLeafNodePid = pid;
	return readError;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf){
	RC writeError = 0;
	RC openError = 0;
	//openError = pf.open(BPTREE,'w');
	if(openError != 0)
		return openError;
	writeError = pf.write(pid, (void*)(buffer));
	//pf.close();
	nonLeafNodePid = pid;
	return writeError;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount(){
	return keyCount;
}

/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid){
	RC insertError = RC_INVALID_ATTRIBUTE;
	int eidCompare,eidCount, keyCompare;
	eidCount = getKeyCount();
	if(eidCount == NON_LEAF_ENTRIES){
		return insertError;
	}
	int firstByte, lastByte;
	insertError = locate(key, eidCompare);
	if(insertError != 0 || eidCount == 0){
		lastByte = eidCount*8;
		insertPid(pid, lastByte);
		insertKey(key, lastByte);
		keyCount++;
	}else{
		keyCompare = getKey(eidCompare);
		if(keyCompare == key){
			return insertError;
		}
		firstByte = eidCompare*8;
		lastByte = eidCount*8-1;
		int copySize = lastByte - firstByte;
		char* tempBuffer = new char[copySize];
		memcpy((void*)tempBuffer, ((void*)(&buffer[firstByte])), copySize);
		insertPid(pid, firstByte);
		insertKey(key, firstByte);
		memcpy(((void*)(&buffer[firstByte+NON_LEAF_ENTRY_SIZE])), (void*)tempBuffer, copySize);
		keyCount++;
	}
	return 0; 
}

RC BTNonLeafNode::locate(int searchKey, int& eid){
	int count = getKeyCount();
	int locateError = -1012;
	for(int i=0; i<count; i++){
		if(searchKey <= getKey(i*8)){
			eid = i;
			locateError = 0;
			break;
		}
	}
	return locateError; 
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey){
	insert(key, pid);
	int moveAmnt;
	if(keyCount%2 == 0){
		moveAmnt = (keyCount/2)-1;
	}else{
		moveAmnt = (keyCount-1)/2;
	}
	int tempKey;
	PageId tempPid;
	midKey = getKey(moveAmnt*8);
	for(int i=moveAmnt; i<keyCount; i++){
		tempKey = getKey(i*8);
		tempPid=getPid(i*8);
		sibling.insert(tempKey,tempPid);
	}
	keyCount = moveAmnt;
	return 0;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid){ 
	int eid=0;
	locate(searchKey, eid);
	pid = getPid(eid*8);
	return 0;
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2){ 
	insertKey(key,0);
	insertPid(pid1,0);
	insertLastPid(pid2);
	return 0;
}
