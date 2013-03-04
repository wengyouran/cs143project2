RC BTreeIndex::insert(int key, const RecordId& rid)
{
    IndexCursor leafCursor;
    locate(key, leafCursor);
    BTLeafNode leaf;
    leaf.read(leafCursor.pid, pf);
    if(leaf.insert(key, rid) == 0)
    {
        leaf.write(leafCursor.pid, pf);
        return 0;
    }
    BTLeafNode sib;
    int sibKey;
    leaf.insertAndSplit(key, rid, sib, sibKey);
    PageId sibPid = pf.endPid();
    sib.setNextNodePtr(leaf.getNextNodePtr());
    leaf.setNextNodePtr(sibPid);
    leaf.write(leafCursor.pid, pf);
    sib.write(sibPid, pf);
    insertInParent(leafCursor.pid, sibKey, sibPid);
}

RC BTreeIndex::insertInParent(PageId leftPid, int midKey, PageId rightPid)
{
    if(leftPid == rootPid)
    {
        BTNonLeafNode root;
        root.setLeftPtr(leftPid);
        root.insert(midKey, rightPid);
        PageId newPid = pf.endPid();
        rootPid = newPid;
        root.write(newPid, pf);
        treeHeight++;
    }
    int height = treeHeight;
    int nodepid = rootPid;
    int parentPid = rootPid;
    BTNonLeafNode nonleaf;
    while(height > 0)
    {
        nonleaf.read(nodepid, pf);
        parentPid = nodepid;
        nonleaf.locateChildPtr(midKey, nodepid);
        if(nodepid == leftPid)
            break;
        height--;
    }
    nonleaf.read(parentPid, pf);
    if(nonleaf.insert(midKey, rightPid) == 0)
    {
        nonleaf.write(parentPid, pf);
        return 0;
    }
    BTNonLeafNode sib;
    int sibKey;
    nonleaf.insertAndSplit(midKey, rightPid, sib, sibKey);
    PageId sibPid = pf.endPid();
    nonleaf.write(parentPid, pf);
    sib.write(sibPid, pf);
    insertInParent(parentPid, sibKey, sibPid);   
}

int BTreeIndex::insertRecursively(int key, const RecordId& rid, int height, PageId currentPid, PageId rightPid){
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
			currentNode.write(nextPid, pf);
			rightPid = nextPid;
			nextPid++;
			key = siblingKey;
			return 1;
		}else{
			currentNode.insert(key, rid);
			return 0;
		}
	}
	BTNonLeafNode currentNode;
	currentNode.read(currentPid, pf);
	PageId leftPid;
	currentNode.locate(key, leftPid);
	height--;
	int x = insertRecursively(key, rid, height, leftPid, rightPid);
	if(x==0){
		return 0;
	}else if{x==1){
		if(currentNode.isNodeFull()){
			BTNonLeafNode siblingNode;
			int siblingKey;
			currentNode.insertAndSplit(key, rid, siblingNode, siblingKey);
			currentNode.write(nextPid, pf);
			rightPid = nextPid;
			nextPid++;
			key = siblingKey;
			return 1;
		}else{
			currentNode.insert(key, rightPid);
			return 0;
		}
	}else{
		return -1;
	}
}