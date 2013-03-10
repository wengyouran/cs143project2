/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeIndex.h"

using namespace std;

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning

  RC     rc;
  int    key;     
  string value;
  int    count;
  int    diff;

  // open the table file
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
    fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
    return rc;
  }
 
  BTreeIndex index;
  if(index.open(table + ".idx", 'r') == 0)    //if index exists
  {
    count = 0;
    int k;
    int lowerBound = -1;
    int upperBound = -1;
    bool isFound = false;
    bool haveLowerBound = false;
    bool haveUpperBound = false;
    int compValue;
    for(k = 0; k < cond.size(); k++)
    {
        if(cond[k].attr == 1 && cond[k].comp != SelCond::NE)
        {
            isFound = true;
            compValue = atoi(cond[k].value);
            switch (cond[k].comp)
            {
                case SelCond::GT:
                case SelCond::GE:
                haveLowerBound = true;
                if(compValue > lowerBound)
                    lowerBound = compValue;
                break;
                case SelCond::LT:
                case SelCond::LE:
                haveUpperBound = true;
                if(upperBound == -1)
                {
                    upperBound = compValue;
                }
                else if(compValue < upperBound)
                {
                    upperBound = compValue;
                }
                break;
                case SelCond::EQ:
                haveLowerBound = haveUpperBound = true;
                upperBound = lowerBound = compValue;
                break;
            }
        }
    }
                      
    if(!isFound)
        goto no_index;
    IndexCursor cursor;
    if(!haveLowerBound)
    {
        index.locate(-1, cursor);
        while((index.readForward(cursor, key, rid) == 0) && (key <= upperBound))
        {
            if ((rc = rf.read(rid, key, value)) < 0) {
              fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
              goto exit_select;
            }
            for (unsigned i = 0; i < cond.size(); i++) {
              // compute the difference between the tuple value and the condition value
              switch (cond[i].attr) {
              case 1:
            diff = key - atoi(cond[i].value);
            break;
              case 2:
            diff = strcmp(value.c_str(), cond[i].value);
            break;
              }

              // skip the tuple if any condition is not met
              switch (cond[i].comp) {
              case SelCond::EQ:
            if (diff != 0) goto index_next_tuple_lt;
            break;
              case SelCond::NE:
            if (diff == 0) goto index_next_tuple_lt;
            break;
              case SelCond::GT:
            if (diff <= 0) goto index_next_tuple_lt;
            break;
              case SelCond::LT:
            if (diff >= 0) goto index_next_tuple_lt;
            break;
              case SelCond::GE:
            if (diff < 0) goto index_next_tuple_lt;
            break;
              case SelCond::LE:
            if (diff > 0) goto index_next_tuple_lt;
            break;
              }
            }

            // the condition is met for the tuple.
            // increase matching tuple counter
            count++;
            
            // print the tuple
            switch (attr) {
            case 1:  // SELECT key
              fprintf(stdout, "%d\n", key);
              break;
            case 2:  // SELECT value
              fprintf(stdout, "%s\n", value.c_str());
              break;
            case 3:  // SELECT *
              fprintf(stdout, "%d '%s'\n", key, value.c_str());
              break;
            }
            index_next_tuple_lt:
            int rubbish;
        }
    }
    else if(!haveUpperBound)
    {
        index.locate(lowerBound, cursor);
        while(index.readForward(cursor, key, rid) == 0)
        {
            if ((rc = rf.read(rid, key, value)) < 0) {
              fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
              goto exit_select;
            }
            for (unsigned i = 0; i < cond.size(); i++) {
              // compute the difference between the tuple value and the condition value
              switch (cond[i].attr) {
              case 1:
            diff = key - atoi(cond[i].value);
            break;
              case 2:
            diff = strcmp(value.c_str(), cond[i].value);
            break;
              }

              // skip the tuple if any condition is not met
              switch (cond[i].comp) {
              case SelCond::EQ:
            if (diff != 0) goto index_next_tuple_gt;
            break;
              case SelCond::NE:
            if (diff == 0) goto index_next_tuple_gt;
            break;
              case SelCond::GT:
            if (diff <= 0) goto index_next_tuple_gt;
            break;
              case SelCond::LT:
            if (diff >= 0) goto index_next_tuple_gt;
            break;
              case SelCond::GE:
            if (diff < 0) goto index_next_tuple_gt;
            break;
              case SelCond::LE:
            if (diff > 0) goto index_next_tuple_gt;
            break;
              }
            }

            // the condition is met for the tuple.
            // increase matching tuple counter
            count++;
            // print the tuple
            switch (attr) {
            case 1:  // SELECT key
              fprintf(stdout, "%d\n", key);
              break;
            case 2:  // SELECT value
              fprintf(stdout, "%s\n", value.c_str());
              break;
            case 3:  // SELECT *
              fprintf(stdout, "%d '%s'\n", key, value.c_str());
              break;
            }
            index_next_tuple_gt:
            int rubbish;
        }
    }
    else
    {
        index.locate(lowerBound, cursor);
        while((index.readForward(cursor, key, rid) == 0) && (key <= upperBound))
        {
            if ((rc = rf.read(rid, key, value)) < 0) {
              fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
              goto exit_select;
            }
            for (unsigned i = 0; i < cond.size(); i++) {
              // compute the difference between the tuple value and the condition value
              switch (cond[i].attr) {
              case 1:
            diff = key - atoi(cond[i].value);
            break;
              case 2:
            diff = strcmp(value.c_str(), cond[i].value);
            break;
              }

              // skip the tuple if any condition is not met
              switch (cond[i].comp) {
              case SelCond::EQ:
            if (diff != 0) goto index_next_tuple_eq;
            break;
              case SelCond::NE:
            if (diff == 0) goto index_next_tuple_eq;
            break;
              case SelCond::GT:
            if (diff <= 0) goto index_next_tuple_eq;
            break;
              case SelCond::LT:
            if (diff >= 0) goto index_next_tuple_eq;
            break;
              case SelCond::GE:
            if (diff < 0) goto index_next_tuple_eq;
            break;
              case SelCond::LE:
            if (diff > 0) goto index_next_tuple_eq;
            break;
              }
            }

            // the condition is met for the tuple.
            // increase matching tuple counter
            count++;
            // print the tuple
            switch (attr) {
            case 1:  // SELECT key
              fprintf(stdout, "%d\n", key);
              break;
            case 2:  // SELECT value
              fprintf(stdout, "%s\n", value.c_str());
              break;
            case 3:  // SELECT *
              fprintf(stdout, "%d '%s'\n", key, value.c_str());
              break;
            }
            index_next_tuple_eq:
            int rubbish;
        }
    }

    if (attr == 4) {
        fprintf(stdout, "%d\n", count);
    }
    rc = 0;
    index.close();
        
    
  }
  else
  {
    no_index:
      // scan the table file from the beginning
      rid.pid = rid.sid = 0;
      count = 0;
      while (rid < rf.endRid()) {
        // read the tuple
        if ((rc = rf.read(rid, key, value)) < 0) {
          fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
          goto exit_select;
        }

        // check the conditions on the tuple
        for (unsigned i = 0; i < cond.size(); i++) {
          // compute the difference between the tuple value and the condition value
          switch (cond[i].attr) {
          case 1:
        diff = key - atoi(cond[i].value);
        break;
          case 2:
        diff = strcmp(value.c_str(), cond[i].value);
        break;
          }

          // skip the tuple if any condition is not met
          switch (cond[i].comp) {
          case SelCond::EQ:
        if (diff != 0) goto next_tuple;
        break;
          case SelCond::NE:
        if (diff == 0) goto next_tuple;
        break;
          case SelCond::GT:
        if (diff <= 0) goto next_tuple;
        break;
          case SelCond::LT:
        if (diff >= 0) goto next_tuple;
        break;
          case SelCond::GE:
        if (diff < 0) goto next_tuple;
        break;
          case SelCond::LE:
        if (diff > 0) goto next_tuple;
        break;
          }
        }

        // the condition is met for the tuple.
        // increase matching tuple counter
        count++;

        // print the tuple
        switch (attr) {
        case 1:  // SELECT key
          fprintf(stdout, "%d\n", key);
          break;
        case 2:  // SELECT value
          fprintf(stdout, "%s\n", value.c_str());
          break;
        case 3:  // SELECT *
          fprintf(stdout, "%d '%s'\n", key, value.c_str());
          break;
        }

        // move to the next tuple
        next_tuple:
        ++rid;
      }

      // print matching tuple count if "select count(*)"
      if (attr == 4) {
        fprintf(stdout, "%d\n", count);
      }
      rc = 0;
  }

 
 
  // close the table file and return
  exit_select:
  rf.close();
  return rc;
}
RC SqlEngine::load(const string& table, const string& loadfile, bool index){
	RecordFile* recordfile = new RecordFile(table +".tbl",'w');
	ifstream ifs;
	ifs.open(loadfile.c_str());
	string buff;
    if(index == false){
		  while(ifs.good()&&getline(ifs,buff,'\n')){
			  RecordId rid;
			  int key;
			  string m_string;
			  if(parseLoadLine(buff,key,m_string)==0){
				  if (recordfile->append(key,m_string,rid)==0){
					
				  }else{
					  return RC_FILE_WRITE_FAILED;
				  }
			  }else{
				  return RC_INVALID_ATTRIBUTE;
			  }
		  }  
    }else{
		  string pfname = table + ".idx";
		  BTreeIndex index;
          index.open(pfname, 'w');
		  while(ifs.good()&&getline(ifs,buff,'\n')){
			  RecordId rid;
			  int key;
			  string m_string;
			  if(parseLoadLine(buff,key,m_string)==0){
				  if (recordfile->append(key,m_string,rid)==0){
					  index.insert(key, rid);
				  }else{
					  return RC_FILE_WRITE_FAILED;
				  }
			  }else{
				  return RC_INVALID_ATTRIBUTE;
			  }
		  }
		  index.close();
    }
	recordfile->close();
	delete(recordfile);
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }

    // get the integer key value
    key = atoi(s);

    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) { 
        value.erase();
        return 0;
    }

    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }

    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }

    return 0;
}
