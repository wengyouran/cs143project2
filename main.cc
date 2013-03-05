/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include "Bruinbase.h"
#include "SqlEngine.h"
#include "PageFile.h"
#include "BTreeNode.h"
#include "BTreeIndex.h"



int main()
{
  // run the SQL engine taking user commands from standard input (console).
  //SqlEngine::run(stdin);
	BTreeIndex myTree = BTreeIndex();
	//char buffer[PageFile::PAGE_SIZE];
	RecordId rid;
	rid.pid = 1;
	rid.sid = 1;
	int key = 1;
	myTree.open("matin",'w');
	//myTree.insert(key,rid);
	
	for(int i=0; i<166; i++){
		myTree.insert(key,rid);
		key+=2;
	}
	key = 2;
	for(int i=0; i<84; i++){
		myTree.insert(key,rid);
		key+=2;
	}
	myTree.printKeys();
	myTree.close();
  return 0;
}
