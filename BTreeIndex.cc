/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"
#include <iostream>

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex(){
    	rootPid = -1;
	treeHeight=0;
	//pf=PageFile("matin",'w');
	
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode){
	RC openError=0, writeError=0, readError = 0;
	char buffer[PageFile::PAGE_SIZE];
	PageId   treePid = 0;
	int pageIdIndex = 0;
	int treeHeightIndex = 4;
	
	openError = pf.open(indexname,mode);
	nextPid = pf.endPid();
	
	if(openError != 0)
		return openError;
	if(mode == 'w'){
		memcpy((void*)(&buffer[pageIdIndex]), (void*)&(rootPid), sizeof(PageId)); 
		memcpy((void*)(&buffer[treeHeightIndex]), (void*)&(treeHeight), sizeof(int)); 
		writeError = pf.write(treePid,(void*)buffer);
		nextPid = pf.endPid();
	}else if(mode == 'r'){
		readError = pf.read(treePid, (void*)&buffer);
		rootPid = *((int*) (buffer));
		treeHeight = *((int*) (buffer+4));
		/*
		memcpy((void*)&(rootPid), (void*)(&buffer[pageIdIndex]), sizeof(PageId)); 
		memcpy((void*)&(treeHeight), (void*)(&buffer[treeHeightIndex]), sizeof(int)); 
		*/
	}else{
		return RC_INVALID_FILE_MODE;
	}
	
	return writeError + readError;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close(){
	RC closeError=0, writeError=0;
	int pageIdIndex = 0;
	int treeHeightIndex = 4;
	PageId   treePid = 0;
	char buffer[PageFile::PAGE_SIZE];
	memcpy((void*)(&buffer[pageIdIndex]), (void*)&(rootPid), sizeof(PageId)); 
	memcpy((void*)(&buffer[treeHeightIndex]), (void*)&(treeHeight), sizeof(int)); 
	writeError = pf.write(treePid,(void*)buffer);
	if(writeError != 0)
		return writeError;
	closeError = pf.close();
    return closeError;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
int BTreeIndex::insertRecursively(int &key, const RecordId& rid, int &height, PageId &currentPid, PageId &rightPid){
	/*
		Base Case: Reached the leaf node
	*/
	if(height == 1){
		BTLeafNode currentNode;
		currentNode.read(currentPid, pf);
		if(currentNode.isNodeFull()){
			BTLeafNode siblingNode;
			int siblingKey;
			currentNode.insertAndSplit(key, rid, siblingNode, siblingKey);
			siblingNode.setNextNodePtr(currentNode.getNextNodePtr());
			currentNode.setNextNodePtr(nextPid);
			currentNode.write(currentNode.getNodeId(), pf);
			siblingNode.write(nextPid, pf);
			rightPid = nextPid;
			nextPid = pf.endPid();
			key = siblingKey;
			return 1;
		}else{
			currentNode.insert(key, rid);
			currentNode.write(currentPid,pf);
			return 0;
		}
	}
	BTNonLeafNode currentNode;
	currentNode.read(currentPid, pf);
	PageId leftPid;
	currentNode.locateChildPtr(key, leftPid);
	height--;
	int x = insertRecursively(key, rid, height, leftPid, rightPid);
	if(x==0){
		return 0;
	}else if(x==1){
		if(currentNode.isNodeFull()){
			BTNonLeafNode siblingNode;
			int siblingKey;
			//siblingNode.initializeRoot(leftPid,key,rightPid);
			currentNode.insertAndSplit(key, nextPid, siblingNode, siblingKey);
			siblingNode.initializeRoot(leftPid,key,rightPid);
			currentNode.write(currentNode.getNodeId(), pf);
			siblingNode.write(nextPid, pf);
			rightPid = nextPid;
			nextPid = pf.endPid();
			key = siblingKey;
			return 1;
		}else{
			currentNode.insert(key, rightPid);
			currentNode.write(currentPid,pf);
			return 0;
		}
	}else{
		return -1;
	}
}

RC BTreeIndex::insert(int key, const RecordId& rid){
	/*
		1) Create root leaf node
		2) insert into the root leaf node
		3) Write the leaf node into the pagefile
		4) keep track of which pagefiles were used with nextPid
	*/
	if(treeHeight <= 0){
		BTLeafNode initialNode = BTLeafNode();
		initialNode.insert(key, rid);
		rootPid = nextPid;
		initialNode.write(rootPid, pf);
		nextPid = pf.endPid();
		treeHeight++;
	}else{
		PageId leftPid = rootPid;
		PageId rightPid;
		int height = treeHeight;
		int x = insertRecursively(key,rid,height, leftPid, rightPid);
		if(x==0){
			return 0;
		}else if(x==1){
			BTNonLeafNode parentNode = BTNonLeafNode();
			parentNode.initializeRoot(leftPid, key, rightPid);
			parentNode.write(nextPid, pf);
			rootPid = parentNode.getNodeId();
			nextPid = pf.endPid();
			treeHeight++;
			return 0;
		}else{
			return -1;
		}
	}
	return 0;
}

/*
 * Find the leaf-node index entry whose key value is larger than or 
 * equal to searchKey, and output the location of the entry in IndexCursor.
 * IndexCursor is a "pointer" to a B+tree leaf-node entry consisting of
 * the PageId of the node and the SlotID of the index entry.
 * Note that, for range queries, we need to scan the B+tree leaf nodes.
 * For example, if the query is "key > 1000", we should scan the leaf
 * nodes starting with the key value 1000. For this reason,
 * it is better to return the location of the leaf node entry 
 * for a given searchKey, instead of returning the RecordId
 * associated with the searchKey directly.
 * Once the location of the index entry is identified and returned 
 * from this function, you should call readForward() to retrieve the
 * actual (key, rid) pair from the index.
 * @param key[IN] the key to find.
 * @param cursor[OUT] the cursor pointing to the first index entry
 *                    with the key value.
 * @return error code. 0 if no error.
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor){
	BTNonLeafNode nonleaf;
	BTLeafNode leaf;
	int readError, pidError, locateError;
	int eid;
	PageId pid = rootPid;
	/*
	Traverse NonLeafNodes if treeHeight is greater than 1
	*/
	if(treeHeight > 1){
		for(int i=0; i<treeHeight-1;i++){
			readError = nonleaf.read(pid, pf);
			if(readError<0){
				return readError;
			}
			pidError = nonleaf.locateChildPtr(searchKey, pid);
			if(pidError<0){
				return pidError;
			}
		}
	}
	/*
	Search leaf node for proper eid
	*/
	readError = leaf.read(pid, pf);
	if(readError<0){
		return readError;
	}
	locateError = leaf.locate(searchKey, eid);
	if(locateError<0){
		return locateError;
	}
	cursor.pid = pid;
	cursor.eid = eid;
	return 0;
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid){
	PageId  cursorPid = cursor.pid;
	int     cursorEid = cursor.eid;
	char buffer[PageFile::PAGE_SIZE];
	BTLeafNode leafPt;
	int readError = leafPt.read(cursorPid, pf);
	if(readError<0){
		return readError;
	}
	readError = leafPt.readEntry(cursorEid, key, rid);
	if(readError<0){
		return readError;
	}
	if(cursorEid >= leafPt.getKeyCount()-1){
		cursor.pid = leafPt.getNextNodePtr();
		if(cursor.pid == 0){
		  return RC_INVALID_PID;
		}
		cursor.eid = 0;
	}else{
		cursor.eid++;
	}
    return 0;
}

/*Test Functions*/
int BTreeIndex::printKeysR(int& height, int& pid){
	if(height == 1){
		BTLeafNode leaf;
		leaf.read(pid,pf);
		leaf.printKeys();
		return 0;
	}
	BTNonLeafNode nonleaf;
	
	nonleaf.read(pid,pf);
	height = height -1;
	for(int i=0; i<=nonleaf.getKeyCount();i++){
		pid = nonleaf.pidAt(i);
		//cout<<"PRINTING FROM PID = " << pid<<" AT i = "<<i<<" with Key Count of "<<endl;
		printKeysR(height,pid);
	}
	return 0;
	
}
int BTreeIndex::printInOrder(PageId somePid){
	BTNonLeafNode nonleaf;
	PageId pid = somePid;
	int height = treeHeight;
	while(height>1){
		nonleaf.read(pid,pf);
		pid = nonleaf.pidAt(0);
		height--;
	}
	BTLeafNode leaf;
	int i = 0;
	while(pid != 0){
		leaf.read(pid,pf);
		//cout<<"PRINTING FROM PID = " << pid<<" AT i = "<<i<<" with Key Count of ";
		leaf.printKeys();
		pid = leaf.getNextNodePtr();
		i++;
	}
	
}
int BTreeIndex::printKeys(){
	int height = treeHeight;
	int pid = rootPid;
	//return printKeysR(height, pid);
	return printInOrder(pid);
}
