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
	keyCount++;
	memcpy((void*)(&buffer[PageFile::PAGE_SIZE-8]), (void*)&(keyCount), sizeof(int)); 
	return 0;
}
bool BTLeafNode::isNodeFull(){
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
	if(openError != 0)
		return openError;
	readError = pf.read(pid, (void*)buffer);
	memcpy((int*)&(keyCount), (int*)(&buffer[PageFile::PAGE_SIZE-8]), sizeof(int)); 	
	leafNodePid = pid;
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
	if(openError != 0)
		return openError;
	memcpy((int*)(&buffer[PageFile::PAGE_SIZE-8]),(int*)&(keyCount), sizeof(int));
	writeError = pf.write(pid, (void*)(buffer));
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
		lastByte = eidCount*LEAF_ENTRY_SIZE;
		insertRecordId(rid, lastByte);
		insertKey(key, lastByte);
		keyCount++;
	}else{
		keyCompare = getKey(eidCompare*LEAF_ENTRY_SIZE);
		if(keyCompare == key){
			return insertError;
		}
		firstByte = eidCompare*LEAF_ENTRY_SIZE;
		lastByte = eidCount*LEAF_ENTRY_SIZE;
		int copySize = lastByte - firstByte;
		char* tempBuffer = new char[copySize];
		memcpy((void*)tempBuffer, (void*)(&buffer[firstByte]), copySize);
		insertRecordId(rid, firstByte);
		insertKey(key, firstByte);
		memcpy((void*)(&buffer[firstByte+LEAF_ENTRY_SIZE]), (void*)tempBuffer, copySize);	
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
	siblingKey = getKey(moveAmnt*LEAF_ENTRY_SIZE);
	for(int i=moveAmnt; i<keyCount; i++){
		tempKey = getKey(i*LEAF_ENTRY_SIZE);
		getRecord(i*LEAF_ENTRY_SIZE, tempRid);
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
		if(searchKey < getKey(i*LEAF_ENTRY_SIZE)){
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
	return *((int*)(&buffer[PageFile::PAGE_SIZE-4]));
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
	return (keyCount >= (NON_LEAF_ENTRIES-1));
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
	if(openError != 0)
		return openError;
	readError = pf.read(pid, (void*)buffer);
	memcpy((void*)&(keyCount), (void*)(&buffer[PageFile::PAGE_SIZE-8]), sizeof(int)); 
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
	if(openError != 0)
		return openError;
	memcpy((void*)(&buffer[PageFile::PAGE_SIZE-8]),(void*)&(keyCount), sizeof(int)); 
	writeError = pf.write(pid, (void*)(buffer));
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
	if(eidCount == 0){
		lastByte = eidCount*NON_LEAF_ENTRY_SIZE;
		insertPid(pid, lastByte);
		insertKey(key, lastByte);
		keyCount++;
	}else if(insertError < 0){
		lastByte = eidCount*NON_LEAF_ENTRY_SIZE;
		insertPid(getLastPid(), lastByte);
		insertKey(key, lastByte);
		insertLastPid(pid);
		keyCount++;
	}else{
		keyCompare = getKey(eidCompare);
		if(keyCompare == key){
			return insertError;
		}
		firstByte = eidCompare*NON_LEAF_ENTRY_SIZE;
		lastByte = eidCount*NON_LEAF_ENTRY_SIZE;
		int copySize = lastByte - firstByte;
		char* tempBuffer = new char[copySize];
		memcpy((void*)tempBuffer, ((void*)(&buffer[firstByte])), copySize);
		insertPid(pid, firstByte);
		insertKey(key, firstByte);
		memcpy(((void*)(&buffer[firstByte+NON_LEAF_ENTRY_SIZE])), (void*)tempBuffer, copySize);
		int x = getPid(firstByte+NON_LEAF_ENTRY_SIZE);//Magic Code
		insertPid(x, firstByte);//Magic Code
		insertPid(pid, firstByte+NON_LEAF_ENTRY_SIZE);//Magic Code
		keyCount++;
	}
	return 0; 
}

RC BTNonLeafNode::locate(int searchKey, int& eid){
	int count = getKeyCount();
	int locateError = -1012;
	for(int i=0; i<count; i++){
		if(searchKey < getKey(i*NON_LEAF_ENTRY_SIZE)){
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
	midKey = getKey(moveAmnt*NON_LEAF_ENTRY_SIZE);
	//cout<<"Amount being moved is "<< moveAmnt<<" && Midkey is "<< midKey<< endl;
	for(int i=moveAmnt+1; i<keyCount; i++){
		tempKey = getKey(i*NON_LEAF_ENTRY_SIZE);
		tempPid=getPid(i*NON_LEAF_ENTRY_SIZE);
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
	if(locate(searchKey, eid) < 0 ){
		pid = getLastPid();
	}else{
		pid = getPid(eid*NON_LEAF_ENTRY_SIZE);
	}
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
	keyCount=1;
	return 0;
}

/*Test Functions*/
/*int BTLeafNode::pidAt(int x){
	
}*/
int BTNonLeafNode::pidAt(int x){
	if(x<keyCount){
		return getPid(x*NON_LEAF_ENTRY_SIZE);
	}else if(x==keyCount){
		return getLastPid();
	}
}
int BTLeafNode::printKeys(){
	int limit = getKeyCount()*LEAF_ENTRY_SIZE;
	//cout<<getKeyCount()<<endl;
	for(int i=8; i<limit; i+=LEAF_ENTRY_SIZE){
		//cout<<*((int*)(&buffer[i]))<<", ";
	}
	//cout<<endl;
}
/*int BTNonLeafNode::printKeys(){
	
}*/
