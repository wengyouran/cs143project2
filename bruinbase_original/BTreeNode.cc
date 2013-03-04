#include "BTreeNode.h"

using namespace std;
const string BPTREE = "B+Tree";
const int ENTRY_SIZE = 12;
const int NUM_ENTRIES = 85; //(PAGE_SIZE-4)/ENTRY_SIZE;

/*Helper Functions*/
int getKey(int location){
	return (*((int*)(&buffer[location+8])));
}
int insertKey(int x, int location){
	memcpy((void*)(&buffer[location+8]), (void*)&(x), sizeof(int)); 
	return 0;
}
int insertRecordId(const RecordId rid, int location){
	memcpy((void*)(&buffer[location]), (void*)&rid, sizeof(RecordId));
	return 0;
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf){ 
		
	RC readError = 0;
	RC openError = 0;
	openError = pf->open(BPTREE,'r');
	if(openError != 0)
		return openError;
	readError = pf->read(pid, (void*)&buffer);
	pf->close();
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
	openError = pf->open(BPTREE,'w');
	if(openError != 0)
		return openError;
	writeError = pf->write(pid, (void*)(&buffer));
	pf->close();
	return writeError; 
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount(){ 
	/*int i = 0;
	int count = 0;
	for(i=ENTRY_SIZE-1; i<PAGE_SIZE-4; i+=ENTRY_SIZE){
		if(buffer[i] == NULL)
			break;
		count++;
	}
	return count;*/
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
	if(eidCount == NUM_ENTRIES){
		return insertError;
	}
	int firstByte, lastByte;
	insertError = locate(key, eidCompare);
	if(insertError != 0 || eidCount == 0){
		lastByte = eidCount*12;
		insertRecordId(rid, lastByte);
		insertKey(key, lastByte);
	}else{
		keyCompare = getKey(eidCompare);
		if(keyCompare == key){
			return insertError;
		}
		firstByte = eidCompare*12;
		lastByte = eidCount*12-1;
		int copySize = lastByte - firstByte;
		char* tempBuffer = new char[copySize];
		memcpy((void*)tempBuffer, (void*)((int)(&buffer)+firstByte), copySize);
		insertRecordId(rid, firstByte);
		insertKey(key, firstByte);
		memcpy((void*)((int)(&buffer)+firstByte+ENTRY_SIZE), (void*)tempBuffer, copySize);
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
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{ return 0; }

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
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{ return 0; }

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ return 0; }

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{ return 0; }

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{ return 0; }
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{ return 0; }

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{ return 0; }


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ return 0; }

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
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ return 0; }

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{ return 0; }

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{ return 0; }
