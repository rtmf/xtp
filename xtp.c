/*
XTP Tagged Document Scripting
Recovered many years after a hard drive crash thanks to archive.org
Copyright (C) 2002-2019  Quinn Morrighan Storm (Rainbow: the Mad Faerie)
	<rtmf@beautifulsunrise.org> <livinglatexkali@gmail.com>
	(I was formerly named Thomas Kenneth Morrow (Jr),
	 listed here in 2012 as "Thomas Mororw (Trinn)"))
	 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <time.h>
#include <sys/types.h>
//#ifdef WINDOWS
#ifdef _WIN32
#include <winsock.h>
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#else
#error "Currently no sockets header ifdef for your OS, please upgrade project to autotools/etc. or add your OS to the #ifdef here."
#endif

using std::string;
using std::cout;
using std::cin;
/*
TAG, contains:
Attributes.
Tag Name
Attributes contain:
Name:Value pairs.
Name:Value pairs contain:
a string Name
a string/TAG list Value
*/
struct xtpAttribute;
struct xtpNode;
struct xtdKey;
struct xtdValue;
struct xtdQuery;
struct xtdRecordPtr;
struct xtpNode
{
    //it's a TAG if attributes is not null, otherwise it's a string.
    xtpAttribute * attributes;
    string value; // either name of tag or simple text if attributes is NULL
    xtpNode * next;
};
struct xtpAttribute
{
    string name;
    xtpNode * value;
    xtpAttribute * next;
};
struct xtdKey
{
	string recordID;
	xtdValue * value;
	xtdKey * next;
};
struct xtdValue
{
	string name;
	string value;
	xtdValue * next;
};
struct xtdRecordPtr
{
	xtdKey * record;
	xtdRecordPtr * next;
};

#define mNULL 0
#define mIS 1
#define mLIKE 2
#define mGREATER 3
#define mLESS 4
#define mNGREATER 5
#define mNLESS 6
#define mNOT 8

#define cNULL 0
#define cAND 1
#define cOR 2
#define cXOR 3

struct xtdQuery
{
	string name;
	string value;
	int mode; // mode, IS, LIKE, GREATER, LESS, NGREATER, NLESS, can be ||d with NOT
	int coupling; // coupling to next (set to 0 if next is NULL), AND, OR, XOR
	xtdQuery * next;
};

struct rexp
{
	int type; // rBEGIN, rSTRING, rQ, rSTAR
	rexp * next;
	string value;
};

#define rBEGIN 0
#define rSTRING 1
#define rQ 2
#define rSTAR 3
#define rMULTI 4
#define rNMULTI 5

string currDB;
xtdKey * dbRoot;
xtdRecordPtr * queryResult;
string idRet;

std::ifstream xread;
std::ofstream xwrite;

rexp * rge;
xtpAttribute * addAttributesFromString(xtpAttribute * base,string str);
xtpAttribute * addAttributesFromStream(xtpAttribute * base,std::istream xin);
xtpAttribute * handleMultipart(string boundary,xtpAttribute * base);

string sockGet(string htt);
string eScape(string in);
string deScape(string in);
void attatch(xtpAttribute * onto,xtpAttribute * what);
string outputNode(xtpNode * nodeToOutput);
string outputNode2(xtpNode * nodeToOutput,int tabify);
string outputNode3(xtpNode * nodeToOutput,int tabify);
xtpNode * findFunction(xtpAttribute * functions,string functionName);
void addFunction(xtpAttribute * functions,xtpNode * funcTop);
xtpNode * execute(xtpNode * root,xtpAttribute * functions,xtpAttribute * environ);
xtpNode * varReplace(xtpNode * root,xtpAttribute * environ);
xtpNode * delNode(xtpNode * root,xtpNode * delAfter);
void copyNodes(xtpAttribute * to,xtpAttribute * from);
void copyAttrs(xtpNode * to,xtpNode * from);
xtpNode * insFunc(xtpNode * root,xtpNode * insertAfter,xtpNode * nodeToInsert,xtpAttribute * environ);
void barf(string junk);
xtpNode * tagify(string curTag,std::istream xin);
//xtpNode * findChild(xtpNode * start, string name);
xtpAttribute * doChildren(string tagName,std::istream xin);
xtpAttribute * doParam(string paramName,std::istream xin);

string selectDB(string dbFile);
string uniqueID();
string syncDB();
string writeDB();
string updateDB(string recordID,string name,string value); // re-reads the DB with syncDB then sets the value then writes the DB
string queryDB(string dbQuery);
string rmRec(string recordID);
string hash(string input);
xtpNode * dbFunc(string errorFunc,string errors);
xtpNode * dbIDFunc(string errorFunc,string errors);
xtpNode * queryResults(string callbackFunc,string errorFunc,string errors);
string dbCreate();

void regcomp(string regexp);
bool regmatch(string regexp,string test);
void regdecomp();
bool regsub(rexp * rgexp,string regexp,string test);

int parse(void);
int main(int argc,char * argv[]);

xtpAttribute * vars;

bool pretty;

//int main(void);

string htescape(string input)
{
	string outtemp;
	const char * c;
	c=input.c_str();
	outtemp="";
	while(c[0]!='\0')
	{
		switch(c[0])
		{
		case '&':
			outtemp+="&amp;";
			break;
		case '<':
			outtemp+="&lt;";
			break;
		default:
			outtemp+=c[0];
		}
		c++;
	}
	return outtemp;
}

/* string sockGet(string htt)
{
    std::ofstream ofs;
    string ss;
    string ss2;
    string ss3;
    char x[18];
    char buf[257];
    int sockfd;
    int sockerr;
    ss="temp/"+hash(string(itoa(rand(),x,10))+string(itoa(rand(),x,10)))+".stm";

    struct sockaddr_in dest_addr;   // will hold the destination addr

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // do some error checking!
    if (sockfd==-1) 
        ss3="<sockError>no Socket Created</sockError>";
    else
    {
        sockerr=htt.find("//");
        if (sockerr<0) barf("Bad URL in winclude");
        if ((htt.length()-sockerr-2)<=0) barf("Bad URL in winclude");
        ss2=htt.substr(sockerr+2,htt.length()-sockerr-2);
        sockerr=ss2.find("/");
        if (sockerr<0) barf("Bad URL in winclude");
        if ((ss2.length()-sockerr-1)<=0) ss3=""; else ss3=ss2.substr(sockerr+1,ss2.length()-sockerr-1);
        ss2=ss2.substr(0,sockerr-1);
        ss3+='\n';
        ss3="GET "+ss3;

        dest_addr.sin_family = AF_INET;          // host byte order
        dest_addr.sin_port = htons(80);   // short, network byte order
    

        dest_addr.sin_addr.s_addr = inet_addr(gethostbyname(ss2.c_str())->h_addr_list[0]);
        memset(&(dest_addr.sin_zero), '\0', 8);  // zero the rest of the struct

        sockerr=connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
        if (sockerr==-1) 
            ss3="<sockError>Could Not Connect</sockError>";
        else
        {
            send(sockfd,ss3.c_str(),0);
            ss3="";
            while(1) {
                sockerr=read(sockfd,buf,256,0);
                if (sockerr==0) break;
                if (sockerr<0) {ss3="<sockError>Error in Read</sockError>";break;}
                ss3+=buf;
            }
        }
    }
    ofs.open(ss.c_str);
    if (!ofs.fail()) {
        ofs<<ss3;
        ofs.close();
    } else barf("Could not open temp file for socket read");
    return ss;
} */
string sockGet(string htt)
{
    std::ofstream ofs;
    string ss;
    string ss2;
    string ss3;
    char x[18];
    char buf[257];
    SOCKET sockfd;
    int sockerr;
    hostent * hp;

    ss="temp/"+hash(string(itoa(rand(),x,10))+string(itoa(rand(),x,10)))+".stm";

    struct sockaddr_in dest_addr;   // will hold the destination addr  

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // do some error checking!
    if (sockfd==INVALID_SOCKET) 
        ss3="<sockError>no Socket Created</sockError>";
    else
    {
        sockerr=htt.find("//");
        if (sockerr<0) return "<sockError>Bad URL</sockError>";//barf("Bad URL in winclude");
        if ((htt.length()-sockerr-2)<=0) return "<sockError>Bad URL</sockError>"; //barf("Bad URL in winclude");
        ss2=htt.substr(sockerr+2,htt.length()-sockerr-2);
        sockerr=ss2.find("/");
        if (sockerr<0) return "<sockError>Bad URL</sockError>";//barf("Bad URL in winclude");
        if ((ss2.length()-sockerr)<=0) ss3=""; else ss3=ss2.substr(sockerr,ss2.length()-sockerr);
        ss2=ss2.substr(0,sockerr);
        ss3+=" HTTP/1.1\r\nhost:"+ss2+"\r\nUser-agent:Mozilla/5.0 (XTP)\r\nreferer:http://xtp.sourceforge.net/\r\n\r\n";
        ss3="GET "+ss3;

        hp=gethostbyname(ss2.c_str());
        
        if (hp==NULL) return "<sockError>Bad URL</sockError>";//barf("Bad URL in winclude");

        memset(&dest_addr,0,sizeof(dest_addr)); 
        memcpy((char *)&dest_addr.sin_addr, hp->h_addr, hp->h_length); /* set address */ 
        dest_addr.sin_family = hp->h_addrtype; 
        dest_addr.sin_port = htons(80); 
        
        sockerr=connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
        if (sockerr==-1) 
            ss3="<sockError>Could Not Connect</sockError>"+ss+ss2;
        else
        {
            send(sockfd,ss3.c_str(),ss3.length(),0);
            ss3="";
            while(1) {
                memset(buf,0,sizeof(buf));
                sockerr=recv(sockfd,buf,sizeof(buf)-1,0);
                if (sockerr==0) break;
                if (sockerr<0) {ss3="<sockError>Error in Read</sockError>";break;}
                ss3+=buf;
            }
            sockerr=ss3.find("\r\n\r\n");
            if (sockerr>=0) ss3=ss3.substr(sockerr+4,ss3.length()-sockerr-4);
        }
    }
    ofs.open(ss.c_str());
    if (!ofs.fail()) {
        ofs<<ss3;
        ofs.close();
    } else return "<sockError>No Temp File</sockError>";//barf("Could not open temp file for socket read");
    return ss;
}

string hash(string input)
{
	char hashout[5]="\0\0\0\0";
	const char * c;
	char x[18];
	string strTemp;
	int hashpos;
	c=input.c_str();
	hashpos=0;
	while(c[0]!='\0')
	{
		hashout[hashpos]+=c[0];
		hashout[hashpos]^=c[1];
		hashpos=(hashpos+(unsigned char)c[0])&3;
		c++;
	}
	strTemp="";
	for (hashpos=0;hashpos<4;hashpos++)
	{
		if (((unsigned char)hashout[hashpos]&0xF0)==0) strTemp+="0";
		strTemp+=itoa((unsigned char)hashout[hashpos],x,16);
	}
	return strTemp;
}
void deltree(xtpNode * tree)
{
	xtpNode * nTemp;
	xtpNode * nTemp2;
	xtpAttribute * aTemp;
	xtpAttribute * aTemp2;
	if (tree!=NULL) {
		aTemp=tree->attributes;
		//cout.flags(std::ios_base::hex);
		//cout<<"n*"<<tree->value<<'\n';
		while (aTemp!=NULL) {
			aTemp2=aTemp;
			aTemp=aTemp->next;
			nTemp=aTemp2->value;
			//cout<<"a*"<<aTemp2->name<<'\n';
			while (nTemp!=NULL) {
				nTemp2=nTemp;
				nTemp=nTemp->next;
				deltree(nTemp2);
			}
			//cout<<'a'<<long(aTemp2)<<'\n';
			delete aTemp2;
		}
		//cout<<'n'<<long(tree)<<'\n';
		delete tree;
	}
}
void delfulltree(xtpNode * tree)
{
	xtpNode * nTemp;
	xtpNode * nTemp2;
	nTemp=tree;
	while (nTemp!=NULL) {
		nTemp2=nTemp;
		nTemp=nTemp->next;
		deltree(nTemp2);
	}
}
void blanktree(xtpNode * tree)
{
	xtpNode * nTemp;
	xtpNode * nTemp2;
	xtpAttribute * aTemp;
	xtpAttribute * aTemp2;
	aTemp=tree->attributes;
	while (aTemp!=NULL) {
		aTemp2=aTemp;
		aTemp=aTemp->next;
		nTemp=aTemp2->value;
		while (nTemp!=NULL) {
			nTemp2=nTemp;
			nTemp=nTemp->next;
			deltree(nTemp2);
		}
		delete aTemp2;
	}
	tree->attributes=NULL;
	tree->value="";
}
bool regmatch(string regexp,string test)
{
	bool b;
	regcomp(regexp);
	b=regsub(rge,regexp,test);
    regdecomp();
	return b;
}
bool regsub(rexp * rgexp,string regexp,string test)
{
	//tokenized, handles *, ?, [], [^], and simple strings
	char * tesp ;
	char x[18];
	tesp=new char[test.length()+1];
	strcpy(tesp,test.c_str());
	while(true)
	{
		if(rgexp==NULL) return (tesp[0]=='\0');
		switch(rgexp->type) {
		case rSTAR:
			if (rgexp->next==NULL) {
				return true;
			}
			if ((rgexp->next->type != rSTRING) && (rgexp->next->type != rMULTI) && (rgexp->next->type != rNMULTI)) barf("bad REGEXP - "+regexp+" "+string(itoa(rgexp->next->type,x,10)));
			switch(rgexp->next->type) {
			case rSTRING:
				tesp=strstr(tesp,rgexp->next->value.c_str());
				if (tesp==NULL) return false;
				break;
			case rMULTI:
				while (true) {
					if (tesp[0]=='\0') return false;
					if (strchr(rgexp->value.c_str(),tesp[0])!=NULL) break;
					tesp++;
				}
				break;
			case rNMULTI:
				while (true) {
					if (tesp[0]=='\0') return false;
					if (strchr(rgexp->value.c_str(),tesp[0])==NULL) break;
					tesp++;
				}
				break;
			}
			break;
		case rQ:
			if (tesp[0]=='\0') return false;
			tesp++;
			break;
		case rSTRING:
			if (strstr(tesp,rgexp->value.c_str())!=tesp) return false; else tesp+=rgexp->value.length();
			break;
		case rMULTI:
			if (strchr(rgexp->value.c_str(),tesp[0])==NULL) return false; else tesp++;
			break;
		case rNMULTI:
			if (strchr(rgexp->value.c_str(),tesp[0])!=NULL) return false; else tesp++;
			break;
		case rBEGIN:
			break;
		default:
			break;
		}
		rgexp=rgexp->next;
	}
	delete[] tesp;
}


void regdecomp(void)
{
	rexp * rTemp;
	rexp * rTemp2;
	rTemp=rge;
	while (rTemp!=NULL)
	{
		rTemp2=rTemp;
		rTemp=rTemp->next;
		delete rTemp2;
	}
	rge=NULL;
}

void regcomp(string regexp)
{
	int rpos;
	const char * rgexp=regexp.c_str();
	rexp * xTemp;
	rexp * xTemp2;
	xTemp=new rexp;
	xTemp->next=NULL;
	xTemp->value="";
	xTemp->type=rBEGIN;
	xTemp2=xTemp;
	string s;
	s="";
	rpos=0;
	while (rgexp[rpos]!='\0') {
		switch(rgexp[rpos])
		{
		case '\\':
			s+=rgexp[rpos+1];
			rpos++;
			break;
		case '*':
			if (s!="") {
				xTemp->next=new rexp;
				xTemp=xTemp->next;
				xTemp->value=s;
				xTemp->type=rSTRING;
				xTemp->next=NULL;
				s="";
			}
			xTemp->next=new rexp;
			xTemp=xTemp->next;
			xTemp->type=rSTAR;
			xTemp->next=NULL;
			break;
		case '?':
			if (s!="") {
				xTemp->next=new rexp;
				xTemp=xTemp->next;
				xTemp->value=s;
				xTemp->type=rSTRING;
				xTemp->next=NULL;
				s="";
			}
			xTemp->next=new rexp;
			xTemp=xTemp->next;
			xTemp->type=rQ;
			xTemp->next=NULL;
			break;
		case '[':
			if (s!="") {
				xTemp->next=new rexp;
				xTemp=xTemp->next;
				xTemp->value=s;
				xTemp->type=rSTRING;
				xTemp->next=NULL;
				s="";
			}
			rpos++;
			xTemp->next=new rexp;
			xTemp=xTemp->next;
			xTemp->type=rMULTI;
			if (rgexp[rpos]=='^') {
				xTemp->type=rNMULTI;
				rpos++;
			}
			while(true) {
				if (rgexp[rpos]==']') break;
				if (rgexp[rpos]=='\0') break;
				if (rgexp[rpos]=='\\') rpos++;
				s+=rgexp[rpos];
				rpos++;
			}
			if (rgexp[rpos]=='\0') barf("Bad Regexp - "+regexp);
			xTemp->value=s;
			xTemp->next=NULL;
			s="";
			break;
		default:
			s+=rgexp[rpos];
			break;
		}
		rpos++;
	}
	if (s!="") {
		xTemp->next=new rexp;
		xTemp=xTemp->next;
		xTemp->value=s;
		xTemp->type=rSTRING;
		xTemp->next=NULL;
		s="";
	}
	rge= xTemp2;
}

string uniqueID()
{
	long int val;
	bool cont=true;
	xtdKey * kTemp;
    char x[18];
	if (currDB=="") return "No database selected.";
	if (dbRoot==NULL) {
		idRet="0";
		return "";
	} else {
		val=-1;
		while (cont) {
			val++;
			if (val==-1) return "Out of room in database.";
			cont=false;
			kTemp=dbRoot;
			while (kTemp!=NULL) {
				if (atoi(kTemp->recordID.c_str())==val) cont=true;
				kTemp=kTemp->next;
			}
		}
		itoa(val,x,10);
		idRet=x;
		return "";
	}
}

string dbCreate()
{
	string s;
	xtdKey * kTemp;
	s=uniqueID();
	if (s!="") return s;
	s=syncDB();
	if (s!="") return s;
	kTemp=new xtdKey;
	kTemp->recordID=idRet;
	kTemp->next=dbRoot;
	kTemp->value=NULL;
	dbRoot=kTemp;
	return writeDB();
}

string dbEscape(string input)
{
	string s;
	int v;
	const char * q = input.c_str();
	v=0;
	while (q[v]!='\0') {
		if ((q[v]=='=')||(q[v]==';')||(q[v]=='\\')||(q[v]=='!')||(q[v]=='.')||(q[v]==':')||(q[v]=='#')) {
			s+='\\';
		}
		s+=q[v];
		v++;
	}
	return s;
}

string writeDB()
{
	xtdKey * kTemp;
	xtdValue * vTemp;
	std::ofstream xout;
	string s;
	s="db/"+currDB+".xtd";
	xout.open(s.c_str());
	if (!xout.fail()) {
		kTemp=dbRoot;
		while(kTemp!=NULL)
		{
			xout<<'#';
			xout<<dbEscape(kTemp->recordID);
			xout<<';';
			xout<<'\n';
			vTemp=kTemp->value;
			while(vTemp!=NULL)
			{
				xout<<':';
				xout<<dbEscape(vTemp->name);
				xout<<'=';
				xout<<dbEscape(vTemp->value);
				xout<<';';
				xout<<'\n';
				vTemp=vTemp->next;
			}
			xout<<'.';
			xout<<'\n';
			kTemp=kTemp->next;
		}
		xout<<'!';
		xout.close();
		return "";
	} else {
		xout.close();
		return "Could not write database "+currDB+".";
	}
}
string rmRec(string recordID)
{
	xtdKey * kTemp;
	xtdKey * kTemp2;
	xtdValue * vTemp;
	xtdValue * vTemp2;
	string s;
	s=syncDB();
	if (s!="") return s;
	kTemp=dbRoot;
	kTemp2=NULL;
	while (kTemp != NULL)
	{
		if (kTemp->recordID==recordID) {
			vTemp=kTemp->value;
			while (vTemp!=NULL) {
				vTemp2=vTemp;
				vTemp=vTemp->next;
				delete vTemp2;
			}
			if (kTemp2==NULL) {
				dbRoot=kTemp->next;
				delete kTemp;
			} else {
				kTemp2->next=kTemp->next;
				delete kTemp;
			}
			return writeDB();
		}
		kTemp2=kTemp;
		kTemp=kTemp->next;
	}
	return "Record not found.";
}

string queryDBCount(string query)
{
	xtdRecordPtr * rTemp;
	string s;
	int q;
    char x[18];
	s=queryDB(query);
	if (s!="") return s;
	rTemp=queryResult;
	q=0;
	while (rTemp!=NULL)
	{
		q++;
		rTemp=rTemp->next;
	}
	itoa(q,x,10);
	idRet=x;
	return "";
}
string updateDB(string recordID,string name,string value)
{
	xtdKey * kTemp;
	xtdValue * vTemp;
	string s;
	s=syncDB();
	if (s!="") return s;
	kTemp=dbRoot;
	while (kTemp!=NULL) {
		if (kTemp->recordID==recordID) {
			vTemp=kTemp->value;
			while (vTemp!=NULL) {
				if (vTemp->name==name) {
					vTemp->value=value;
					return writeDB();
				}
				vTemp=vTemp->next;
			}
			vTemp = new xtdValue;
			vTemp->name=name;
			vTemp->value=value;
			vTemp->next=kTemp->value;
			kTemp->value=vTemp;
			return writeDB();
		}
		kTemp=kTemp->next;
	}
	return "Record not found.";
}

string selectDB(string dbFile)
{
	std::ifstream xin;
	std::ofstream xout;
	string s;
	s="db/"+dbFile+".xtd";
	xin.open(s.c_str());
    if (!xin.fail()) {
		currDB=dbFile;
		xin.close();
		return syncDB();
	} else {
		xin.close();
		xout.open(s.c_str());
		if (!xout.fail()) {
			currDB=dbFile;
			xout<<'!';
			xout.close();
			return syncDB();
		} else {
			return "Could not select database "+dbFile+".";
		}
	}
}

xtpNode * queryResults (string callbackFunc,string errorFunc,string errors)
{
	xtpNode * nTemp;
	xtpNode * nTemp2;
	xtpNode * nTemp3;
	xtpAttribute * aTemp;
	xtdRecordPtr * rTemp;
	xtdKey * kTemp;
	xtdValue * vTemp;
	if (errors=="") {
		nTemp=new xtpNode;
		nTemp->attributes=NULL;
		nTemp->value="";
		nTemp->next=NULL;
		nTemp2=nTemp;
		rTemp=queryResult;
		while (rTemp!=NULL)
		{
			kTemp=rTemp->record;
			nTemp3=new xtpNode;
			nTemp3->value=callbackFunc;
			nTemp3->next=NULL;
			nTemp3->attributes=new xtpAttribute;
			nTemp3->attributes->name="_recordID";
			nTemp3->attributes->value=new xtpNode;
			nTemp3->attributes->value->value=kTemp->recordID;
			nTemp3->attributes->value->attributes=NULL;
			nTemp3->attributes->value->next=NULL;
			nTemp3->attributes->next=NULL;
			vTemp=kTemp->value;
			while (vTemp!=NULL)
			{
				aTemp=new xtpAttribute;
				aTemp->name=vTemp->name;
				aTemp->value=new xtpNode;
				aTemp->value->value=vTemp->value;
				aTemp->value->attributes=NULL;
				aTemp->value->next=NULL;
				aTemp->next=nTemp3->attributes;
				nTemp3->attributes=aTemp;
				vTemp=vTemp->next;
			}
			nTemp2->next=nTemp3;
			nTemp2=nTemp3;
			rTemp=rTemp->next;
		}
	} else {
		nTemp = new xtpNode;
		nTemp->value=errorFunc;
		nTemp->next=NULL;
		nTemp->attributes=new xtpAttribute;
		nTemp->attributes->name="_body";
		nTemp->attributes->value=new xtpNode;
		nTemp->attributes->value->value=errors;
		nTemp->attributes->value->attributes=NULL;
		nTemp->attributes->value->next=NULL;
		nTemp->attributes->next=NULL;
	}
	return nTemp;
}

xtpNode * dbFunc(string errorFunc,string errors)
{
	xtpNode * nTemp;
	if (errors=="") {
		nTemp=new xtpNode;
		nTemp->attributes=NULL;
		nTemp->value="";
		nTemp->next=NULL;
	} else {
		nTemp=new xtpNode;
		nTemp->value=errorFunc;
		nTemp->next=NULL;
		nTemp->attributes=new xtpAttribute;
		nTemp->attributes->name="_body";
		nTemp->attributes->value=new xtpNode;
		nTemp->attributes->value->value=errors;
		nTemp->attributes->value->attributes=NULL;
		nTemp->attributes->value->next=NULL;
		nTemp->attributes->next=NULL;
	}
	return nTemp;
}
xtpNode * dbIDFunc(string errorFunc,string errors)
{
	xtpNode * nTemp;
	if (errors=="") {
		nTemp=new xtpNode;
		nTemp->attributes=NULL;
		nTemp->value=idRet;
		nTemp->next=NULL;
	} else {
		nTemp=new xtpNode;
		nTemp->value=errorFunc;
		nTemp->next=NULL;
		nTemp->attributes=new xtpAttribute;
		nTemp->attributes->name="_body";
		nTemp->attributes->value=new xtpNode;
		nTemp->attributes->value->value=errors;
		nTemp->attributes->value->attributes=NULL;
		nTemp->attributes->value->next=NULL;
		nTemp->attributes->next=NULL;
	}
	return nTemp;
}

string queryDB(string dbQuery)
{
	//DB Query Format:
	//NOT modifier
	//IS
	//LIKE
	//GREATER
	//LESS
	//NGREATER - atoi
	//NLESS - atoi
	//build the query...
	xtdQuery * q;
	xtdQuery * q2;
	xtdQuery * q3;
	xtdRecordPtr * rTemp;
	xtdRecordPtr * rTemp2;
	xtdKey * kTemp;
	xtdValue * vTemp;
	int pos;
	const char * s=dbQuery.c_str();
	string s2;
	bool cont;
    bool cont2;
    bool mnot;
    cont2=true;
    mnot=false;
	rTemp=queryResult;
    while (rTemp!=NULL)
	{
		rTemp2=rTemp;
		rTemp=rTemp->next;
		delete rTemp2;
	}
	queryResult=NULL;
	q=NULL;
	q2=NULL;
	pos=0;
    while((s[pos]!='\0')&&(cont2)) {
		//get a quote or a paren or a NOT or a \0
		switch (s[pos]) {
		case '"':
			q3=new xtdQuery;
			q3->next=NULL;
            if (mnot) q3->mode=mNOT; else q3->mode=mNULL;
			q3->coupling=cNULL;
			q3->name="";
			q3->value="";
			s2="";
			pos++;
			cont=true;
			while (cont) {
				switch(s[pos])
				{
				case '"':
					cont=false;
					break;
				case '\0':
					return "Malformed query, ended within quoted string.";
					break;
				case '\\':
					if (s[pos+1]!='\0') {
						s2+=s[pos+1];
					} else return "Malformed query, ended within quoted string.";
					pos++;
					break;
				default:
					s2+=s[pos];
				}
				pos++;
			}
			q3->name=s2;
			s2="";
			cont=true;
			while (cont){
				switch(s[pos])
				{
				case '\0':
					return "Malformed query, ended too soon.";
					break;
				case ' ':
					break;
				default:
					cont=false;
					break;
				}
				pos++;
			}
			cont=true;
			pos--;
			while(cont) {
				switch(s[pos])
				{
				case '\0':
					return "Malformed query, ended too soon.";
					break;
				case ' ':
					cont=false;
					break;
				default:
					s2+=s[pos];
					break;
				}
				pos++;
			}
			if (s2=="IS") {q3->mode|=mIS;cont=true;}
			if (s2=="LIKE") {q3->mode|=mLIKE;cont=true;}
			if (s2=="GREATER") {q3->mode|=mGREATER;cont=true;}
			if (s2=="LESS") {q3->mode|=mLESS;cont=true;}
			if (s2=="NGREATER") {q3->mode|=mNGREATER;cont=true;}
			if (s2=="NLESS") {q3->mode|=mNLESS;cont=true;}
			if (!cont) return "Malformed query, no such mode as "+s2+".";
			s2="";
			while(cont) {
				switch(s[pos])
				{
				case '\0':
					return "Malformed query, ended too soon.";
					break;
				case '"':
					cont=false;
					break;
				default:
					break;
				}
				pos++;
			}
			cont=true;
			while (cont) {
				switch(s[pos])
				{
				case '"':
					cont=false;
					break;
				case '\0':
					return "Malformed query, ended within quoted string.";
					break;
				case '\\':
					if (s[pos+1]!='\0') {
						s2+=s[pos+1];
					} else return "Malformed query, ended within quoted string.";
					pos++;
					break;
				default:
					s2+=s[pos];
				}
				pos++;
			}
			q3->value=s2;
			s2="";
			cont=true;
			while (cont){
				switch(s[pos])
				{
				case '\0':
					cont=false;
					cont2=false;
					break;
				case ' ':
					break;
				default:
					cont=false;
					break;
				}
				pos++;
			}
			pos--;
			if (cont2) {
				cont=true;
				while(cont) {
					switch(s[pos])
					{
					case '\0':
						cont=false;
						cont2=false;
						break;
					case ' ':
						cont=false;
						break;
					default:
						s2+=s[pos];
						break;
					}
					pos++;
				}
				if (cont2) {
					cont=false;
					if (s2=="AND") {q3->coupling=cAND;cont=true;}
					if (s2=="OR") {q3->coupling=cOR;cont=true;}
					if (s2=="XOR") {q3->coupling=cXOR;cont=true;}
					if (!cont) return "Malformed query, no such coupling as "+s2+".";
					//q2 is tail, q is head.
					if (q2==NULL) {
						q=q3;
						q2=q;
					} else {
						q2->next=q3;
						q2=q3;
					}
				} else return "Malformed query, ended too soon.";
			} else {
				q3->coupling=cNULL;
				if (q2==NULL) {
					q=q3;
					q2=q;
				} else {
					q2->next=q3;
					q2=q3;
				}
			}
			break;
		case 'N':
            if (!mnot) {
				//get next two chars, see if "O" and "T"
				if (s[pos+1]!='O') return "Malformed query, N encountered followed by something other than O where NOT, quote, expected.";
				if (s[pos+2]!='T') return "Malformed query, NO encountered followed by something other than T where NOT, quote, expected.";
				if (s[pos+3]!=' ') return "Malformed query, NOT encountered followed by something other than SPACE where NOT, quote, expected.";
                mnot=true;
				pos+=3;
			} else {
				return "Malformed query, N encountered after NOT, when quote expected.";
			}
			break;
		default:
			pos++;
			break;
		}
	}
	if ((cont2) && (q!=NULL))return "Malformed query, ended too soon.";
	//if (q==NULL) return "No query.";
	//we -should- now have a query in q.
	//let's apply it record by record.
	queryResult=NULL;
	rTemp2=NULL;
	kTemp=dbRoot;
	while (kTemp!=NULL)
	{
		cont=true;
		pos=cNULL;
		q2=q;
		while (q2!=NULL)
		{
			//find the proper value
			cont2=false;
			if (q2->name=="_recordID") 
			{
				s2=kTemp->recordID;
				cont2=true;
			} else {
				vTemp=kTemp->value;
				while (vTemp!=NULL)
				{
					if (vTemp->name==q2->name) {
						cont2=true;
						s2=vTemp->value;
						break;
					}
					vTemp=vTemp->next;
				}
			}
			if (cont2) {
				switch(q2->mode & 0x07)
				{
				case mIS:
					cont2=(q2->value==s2);
					break;
				case mLIKE:
					cont2=(regmatch(q2->value,s2));
					break;
				case mGREATER:
					cont2=(s2>q2->value);
					break;
				case mLESS:
					cont2=(s2<q2->value);
					break;
				case mNGREATER:
					cont2=(atoi(s2.c_str())>atoi(q2->value.c_str()));
					break;
				case mNLESS:
					cont2=(atoi(s2.c_str())<atoi(q2->value.c_str()));
					break;
				default:
					cont2=true;
				}
				if ((q2->mode & mNOT)==mNOT) cont2=!cont2;
			} else cont2=false;
			switch(pos)
			{
			case cNULL:
				cont=cont2;
				break;
			case cAND:
				cont=(cont&&cont2);
				break;
			case cOR:
				cont=(cont||cont2);
				break;
			case cXOR:
				cont^=cont2;
				break;
			}
			pos=q2->coupling;
			q2=q2->next;
		}
		if (cont) {
			rTemp = new xtdRecordPtr;
			rTemp->record=kTemp;
			rTemp->next=NULL;
			if (rTemp2==NULL) {
				rTemp2=rTemp;
				queryResult=rTemp;
			} else {
				rTemp2->next=rTemp;
				rTemp2=rTemp;
			}
		}
		kTemp=kTemp->next;
	}
	return "";
}

string syncDB()
{
	std::ifstream xin;
	xtdKey * kTemp;
	xtdKey * kTemp2;
	xtdValue * vTemp;
	xtdValue * vTemp2;
	string sTemp;
	string sTemp2;
	char c;
	int cont=true;
	if (currDB=="") {
		return "No database selected.";
	}
	kTemp=dbRoot;
	while (kTemp!=NULL) {
		vTemp=kTemp->value;
		while (vTemp!=NULL) {
			vTemp2=vTemp;
			vTemp=vTemp->next;
			delete vTemp2;
		}
		kTemp2=kTemp;
		kTemp=kTemp->next;
		delete kTemp2;
	}
	sTemp="db/"+currDB+".xtd";
	xin.open(sTemp.c_str());
	dbRoot=NULL;
	kTemp2=NULL;
	if (!xin.fail()) {
		while ((!xin.eof())&&cont)
		{
			xin.get(c);
			switch(c) {
			case '#':
				kTemp = new xtdKey;
				kTemp->next=NULL;
				kTemp->value=NULL;
				kTemp->recordID="";
				vTemp2=NULL;
				sTemp="";
				cont=true;
				while ((!xin.eof())&&cont)
				{
					xin.get(c);
					switch(c) {
					case ';':
						cont=false;
						break;
					case '\\':
						if (!xin.eof()) {
							xin.get(c);
							sTemp+=c;
						} else {
							xin.close();
							return "Input past end of DB "+currDB+".";
						}
						break;
					default:
						sTemp+=c;
						break;
					}
				}
				if (cont) {
					xin.close();
					return "Input past end of DB "+currDB+".";
				}
				kTemp->recordID=sTemp;
				cont=true;
				while ((!xin.eof())&&cont)
				{
					xin.get(c);
					switch(c) {
					case ':':
						vTemp = new xtdValue;
						vTemp->next=NULL;
						vTemp->name="";
						vTemp->value="";
						sTemp="";
						cont=true;
						while ((!xin.eof())&&cont)
						{
							xin.get(c);
							switch(c) {
							case '=':
								vTemp->name=sTemp;
								cont=false;
								break;
							case '\\':
								if (!xin.eof()) {
									xin.get(c);
									sTemp+=c;
								} else {
									xin.close();
									return "Input past end of DB "+currDB+".";
								}
								break;
							default:
								sTemp+=c;
							}
						}
						if (cont) {
							xin.close();
							return "Input past end of DB "+currDB+".";
						}
						cont=true;
						sTemp="";
						while ((!xin.eof())&&cont)
						{
							xin.get(c);
							switch(c) {
							case ';':
								vTemp->value=sTemp;
								cont=false;
								break;
							case '\\':
								if (!xin.eof()) {
									xin.get(c);
									sTemp+=c;
								} else {
									xin.close();
									return "Input past end of DB "+currDB+".";
								}
								break;
							default:
								sTemp+=c;
							}
						}
						if (cont) {
							xin.close();
							return "Input past end of DB "+currDB+".";
						}
						cont=true;
						if (vTemp2==NULL) {
							vTemp2=vTemp;
							kTemp->value=vTemp;
						} else {
							vTemp2->next=vTemp;
							vTemp2=vTemp2->next;
						}
						break;
					case '.':
						cont=false;
						break;
					default:
						break;
					}
				}
				if (cont) {
					xin.close();
					return "Input past end of DB "+currDB+".";
				}
				cont=true;
				if (kTemp2==NULL) {
					kTemp2=kTemp;
					dbRoot=kTemp;
				} else {
					kTemp2->next=kTemp;
					kTemp2=kTemp2->next;
				}
				break;
			case '!':
				cont=false;
				break;
			default:
				break;
			}
		}
		if (cont) {
			xin.close();
			return "Input past end of DB "+currDB+".";
		} else {
			return "";
		}
	} else {
		xin.close();
		return "Could not sync database "+currDB+".";
	}
}

xtpNode * tagify(string crTag,std::istream xin)
{
    xtpNode * startNode;
    xtpNode * thisNode;
    thisNode=NULL;
    xtpAttribute * tempNode;
    char c;
    string thisBlock;
    string curTag;
    int currInTag = 0;
    int cont = 1;
    int cont2;
    thisBlock="";
    curTag=crTag;
    startNode=NULL;
    while((!xin.eof()) && (cont)) {
        xin.get(c);
        if (xin.eof()) break;
        if (c=='<') {
            if (!xin.eof()) {
                xin.get(c);
                if (c!='/') {
                    xin.putback(c);
                    if (currInTag) {
                        //barf(curTag+":No tags allowed on the top level within a tag, tags must be contained within attributes of a tag.");
                        if (thisNode!=NULL) {
                            delfulltree(startNode);
                        }
                        thisNode=new xtpNode;
                        thisNode->attributes=NULL;
                        thisNode->next=NULL;
                        thisNode->value="barf";
                        thisNode->attributes=new xtpAttribute;
                        thisNode->attributes->name="_body";
                        thisNode->attributes->next=NULL;
                        thisNode->attributes->value=new xtpNode;
                        thisNode->attributes->value->attributes=NULL;
                        thisNode->attributes->value->next=NULL;
                        thisNode->attributes->value->value=curTag+":No tags allowed on the top level within a tag, tags must be contained within attributes of a tag.";
                        return thisNode;
                    } else {
                        if (thisBlock!="") {
                            if (thisNode == NULL) {
                                thisNode = new xtpNode;
                                startNode=thisNode;
                            } else {
                                thisNode->next = new xtpNode;
                                thisNode=thisNode->next;
                            }
                            thisNode->value=thisBlock;
                            thisNode->attributes=NULL;
                            thisNode->next=NULL;
                        }
                        thisBlock="";
                    }
                    currInTag=1;
                } else {
                    if (thisBlock!="") {
                        if (thisNode == NULL) {
                            thisNode = new xtpNode;
                            startNode=thisNode;
                        } else {
                            thisNode->next = new xtpNode;
                            thisNode=thisNode->next;
                        }
                        thisNode->value=thisBlock;
                        thisNode->attributes=NULL;
                        thisNode->next=NULL;
                    }
                    int cont2=1;
                    thisBlock="";
                    while((!xin.eof()) && (cont2))
                    {
                        xin.get(c);
                        if (c=='>') {
                            if (thisBlock==curTag) {
                                cont2=0;
                                cont=0;
                            } else {
                                if (thisNode!=NULL) {
                                    delfulltree(startNode);
                                }
                                thisNode=new xtpNode;
                                thisNode->attributes=NULL;
                                thisNode->next=NULL;
                                thisNode->value="barf";
                                thisNode->attributes=new xtpAttribute;
                                thisNode->attributes->name="_body";
                                thisNode->attributes->next=NULL;
                                thisNode->attributes->value=new xtpNode;
                                thisNode->attributes->value->attributes=NULL;
                                thisNode->attributes->value->next=NULL;
                                thisNode->attributes->value->value="Mismatched closing tag: "+curTag+" with "+thisBlock;
                                return thisNode;
                                //barf("Mismatched closing tag: "+curTag+" with "+thisBlock);
                            }
                        } else thisBlock+=c;
                    }
                    thisBlock="";
                    if (cont2) {
                        /*if (thisNode!=NULL) {
                            delfulltree(startNode);
                        }
                        thisNode=new xtpNode;
                        thisNode->attributes=NULL;
                        thisNode->next=NULL;
                        thisNode->value="barf";
                        thisNode->attributes=new xtpAttribute;
                        thisNode->attributes->name="_body";
                        thisNode->attributes->next=NULL;
                        thisNode->attributes->value=new xtpNode;
                        thisNode->attributes->value->attributes=NULL;
                        thisNode->attributes->value->next=NULL;
                        thisNode->attributes->value->value=curTag+":Unexpected end of input.";
                        return thisNode;*/
                        //barf(curTag+":Unexpected end of input.");
                    }
                }
            } else {
                /*if (thisNode!=NULL) {
                    delfulltree(startNode);
                }
                thisNode=new xtpNode;
                thisNode->attributes=NULL;
                thisNode->next=NULL;
                thisNode->value="barf";
                thisNode->attributes=new xtpAttribute;
                thisNode->attributes->name="_body";
                thisNode->attributes->next=NULL;
                thisNode->attributes->value=new xtpNode;
                thisNode->attributes->value->attributes=NULL;
                thisNode->attributes->value->next=NULL;
                thisNode->attributes->value->value=curTag+":Unexpected end of input.";
                return thisNode;*/
                //barf(curTag+":Unexpected end of input.");
            }
        } else
            if ((c=='>') && (currInTag)) {
                tempNode=new xtpAttribute;
                tempNode->name="_body";
                tempNode->next=NULL;
                tempNode->value=tagify(thisBlock,xin);
                if (!thisNode) {
                    thisNode = new xtpNode;
                    startNode=thisNode;
                } else {
                    thisNode->next = new xtpNode;
                    thisNode=thisNode->next;
                }
                thisNode->value=thisBlock;
                thisNode->attributes=tempNode;
                thisNode->next=NULL;
                thisBlock="";
                currInTag=0;
            } else
                if (c=='\\') {
                    xin.get(c);
                    if (xin.eof()) {
                        if (thisNode!=NULL) {
                            delfulltree(startNode);
                        }
                        thisNode=new xtpNode;
                        thisNode->attributes=NULL;
                        thisNode->next=NULL;
                        thisNode->value="barf";
                        thisNode->attributes=new xtpAttribute;
                        thisNode->attributes->name="_body";
                        thisNode->attributes->next=NULL;
                        thisNode->attributes->value=new xtpNode;
                        thisNode->attributes->value->attributes=NULL;
                        thisNode->attributes->value->next=NULL;
                        thisNode->attributes->value->value="Escape char (\\) encountered without something to escape at the end of the file.";
                        return thisNode;
                        //barf("Escape char (\\) encountered without something to escape at the end of the file.");
                    }
                    switch(c) {
                    case 'n':c='\n';break;
                    case 't':c='\t';break;
                    default:break;
                    }
                    thisBlock+=c;
                } else
                    if (((c==' ')||(c=='\n')||(c=='\t')) && (currInTag)) {
                        if ((thisBlock=="meta")||(thisBlock[0]=='!')||(thisBlock[0]=='?')) {
                            while (c!='>' && !xin.eof()) {
                                xin.get(c);
                            }
                            if (c!='>') {
                                /*if (thisNode!=NULL) {
                                    delfulltree(startNode);
                                }
                                thisNode=new xtpNode;
                                thisNode->attributes=NULL;
                                thisNode->next=NULL;
                                thisNode->value="barf";
                                thisNode->attributes=new xtpAttribute;
                                thisNode->attributes->name="_body";
                                thisNode->attributes->next=NULL;
                                thisNode->attributes->value=new xtpNode;
                                thisNode->attributes->value->attributes=NULL;
                                thisNode->attributes->value->next=NULL;
                                thisNode->attributes->value->value=curTag+":Unexpected end of input.";
                                return thisNode;*/
                                //barf("Unexpected end of input");
                            }
                        } else {
                            tempNode=doChildren(thisBlock,xin);
                            if (tempNode != NULL) {
                                if (thisNode == NULL) {
                                    thisNode = new xtpNode;
                                    startNode=thisNode;
                                } else {
                                    thisNode->next = new xtpNode;
                                    thisNode=thisNode->next;
                                }
                                thisNode->value=thisBlock;
                                thisNode->attributes=tempNode;
                                thisNode->next=NULL;
                            }
                        }
                        thisBlock="";
                        currInTag=0;
                    } else  if ((c==' ')||(c=='\n')||(c=='\t')) {
                        if ((c==' ')&&(thisBlock!="")) {
                        /*
                        if (thisNode == NULL) {
                        thisNode = new xtpNode;
                        startNode=thisNode;
                        } else {
                        thisNode->next = new xtpNode;
                        thisNode=thisNode->next;
                        }
                        thisNode->value=thisBlock;
                        thisNode->attributes=NULL;
                        thisNode->next=NULL;
                        thisBlock="";
                        currInTag=0;
                            */
                            if(thisBlock[thisBlock.length()-1]!=' ')
                                thisBlock+=' ';
                        }
                    } else {
                        thisBlock+=c;
                        if ((thisBlock=="!--") && (currInTag)) {
                            cont2=1;
                            while(!xin.eof() && cont2) {
                                xin.get(c);
                                if (xin.eof()) break;
                                thisBlock+=c;
                                if (c=='-') {
                                    if (!xin.eof()) {
                                        xin.get(c);
                                        thisBlock+=c;
                                        if (c=='-') {
                                            if (!xin.eof()) {
                                                xin.get(c);
                                                thisBlock+=c;
                                                if (c=='>') {
                                                    cont2=0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if (thisNode == NULL) {
                                thisNode = new xtpNode;
                                startNode=thisNode;
                            } else {
                                thisNode->next = new xtpNode;
                                thisNode=thisNode->next;
                            }
                            thisNode->value="<"+thisBlock;
                            thisNode->attributes=NULL;
                            thisNode->next=NULL;
                            thisBlock="";
                            currInTag=0;
                        }
                    }
    }
    if ((cont) && (curTag!="")) {
        /*if (thisNode!=NULL) {
            delfulltree(startNode);
        }
        thisNode=new xtpNode;
        thisNode->attributes=NULL;
        thisNode->next=NULL;
        thisNode->value="barf";
        thisNode->attributes=new xtpAttribute;
        thisNode->attributes->name="_body";
        thisNode->attributes->next=NULL;
        thisNode->attributes->value=new xtpNode;
        thisNode->attributes->value->attributes=NULL;
        thisNode->attributes->value->next=NULL;
        thisNode->attributes->value->value=curTag+":Unexpected end of input.";
        return thisNode;*/
        //barf(curTag+":Unexpected end of input.");
    }
    if (thisBlock!="") {
        if (thisNode == NULL) {
            thisNode = new xtpNode;
            startNode=thisNode;
        } else {
            thisNode->next = new xtpNode;
            thisNode=thisNode->next;
        }
        thisNode->value=thisBlock;
        thisNode->attributes=NULL;
        thisNode->next=NULL;
        thisBlock="";
    }
    if (currInTag) {
        //barf("Non-closed tag!");
        if (thisNode!=NULL) {
            delfulltree(startNode);
        }
        thisNode=new xtpNode;
        thisNode->attributes=NULL;
        thisNode->next=NULL;
        thisNode->value="barf";
        thisNode->attributes=new xtpAttribute;
        thisNode->attributes->name="_body";
        thisNode->attributes->next=NULL;
        thisNode->attributes->value=new xtpNode;
        thisNode->attributes->value->attributes=NULL;
        thisNode->attributes->value->next=NULL;
        thisNode->attributes->value->value="Non-closed tag!";
        return thisNode;
    }
    if (thisNode != NULL) {
        thisNode->next=NULL;
    } else {
        thisNode=new xtpNode;
        thisNode->attributes=NULL;
        thisNode->value="";
        thisNode->next=NULL;
        startNode=thisNode;
    } //barf("Absolutely, Positively Nothing.");
    return startNode;
}
xtpAttribute * doChildren(string tagName, std::istream xin)
{
    //do the attributes line for a tag, stopping at the first 'extra' >
    //we are expected to have started just after the tag's name and
    //the first delimiting space
    xtpAttribute * startNode;
    xtpAttribute * thisNode;
    thisNode=NULL;
    startNode = NULL;
    xtpNode * tempNode;
    char c;
    string thisBlock;
    int cont = 1;
    if (tagName=="!--") {
        while(!xin.eof() && cont) {
            xin.get(c);
            if (xin.eof()) break;
            if (c=='-') {
                if (!xin.eof()) {
                    xin.get(c);
                    if (c=='-') {
                        if (!xin.eof()) {
                            xin.get(c);
                            if (c=='>') {
                                cont=0;
                            }
                        }
                    }
                }
            }
        }
        //if (cont) barf("Unexpected end of input.");
        return NULL;
    } else {
        while (!xin.eof() && cont) {
            xin.get(c);
            if (xin.eof()) break;
            if (c=='=') {
                if (thisNode != NULL) {
                    thisNode->next=doParam(thisBlock,xin);
                    thisNode=thisNode->next;
                } else {
                    thisNode=doParam(thisBlock,xin);
                    startNode=thisNode;
                }
                thisBlock="";
            } else
                if (c=='<') {
                    return NULL;
                    barf(tagName+":No tags allowed on the top level within a tag, tags must be contained within attributes of a tag.");
                } else
                    if (c=='\\') {
                        xin.get(c);
                        if (xin.eof()) barf("Escape char (\\) encountered without something to escape at the end of the file.");
                        switch(c) {
                        case 'n':c='\n';break;
                        case 't':c='\t';break;
                        default:break;
                        }
                        thisBlock+=c;
                    } else
                        if ((c==' ')||(c=='\n')||(c=='\t')) {
                            if (thisBlock!="") {
                                if (thisNode != NULL) {
                                    thisNode->next=new xtpAttribute;
                                    thisNode=thisNode->next;
                                } else {
                                    thisNode=new xtpAttribute;
                                    startNode=thisNode;
                                }
                                thisNode->name=thisBlock;
                                thisNode->value=NULL;
                                thisNode->next=NULL;
                                thisBlock="";
                                //barf("You can't have an attribute with no value");
                            }
                        } else
                            if (c=='/') {
                                if (!xin.eof()) {
                                    xin.get(c);
                                    if (c=='>') {
                                        if (thisNode != NULL) {
                                            thisNode->next=new xtpAttribute;
                                            thisNode=thisNode->next;
                                        } else {
                                            thisNode=new xtpAttribute;
                                            startNode=thisNode;
                                        }
                                        tempNode = new xtpNode;
                                        tempNode->value="";
                                        thisNode->name="_body";
                                        thisNode->value=tempNode;
                                        tempNode->attributes=NULL;
                                        thisNode->next=NULL;
                                        tempNode->next=NULL;
                                        cont=0;
                                    } else {
                                        return NULL;
                                        //barf("/ found where not expected.");
                                    }
                                } else {
                                    barf("Unexpected end of input.");
                                }
                            } else
                                if (c=='>') {
                                    if (thisBlock!="") {
                                        if (thisNode != NULL) {
                                            thisNode->next=new xtpAttribute;
                                            thisNode=thisNode->next;
                                        } else {
                                            thisNode=new xtpAttribute;
                                            startNode=thisNode;
                                        }
                                        thisNode->name=thisBlock;
                                        thisNode->value=NULL;
                                        thisNode->next=NULL;
                                        thisBlock="";
                                        //barf("You can't have an attribute with no value");
                                    }
                                    tempNode=tagify(tagName,xin);
                                    if (thisNode != NULL) {
                                        thisNode->next=new xtpAttribute;
                                        thisNode=thisNode->next;
                                    } else {
                                        thisNode=new xtpAttribute;
                                        startNode=thisNode;
                                    }
                                    thisNode->name="_body";
                                    thisNode->value=tempNode;
                                    thisNode->next=NULL;
                                    cont=0;
                                } else {
                                    thisBlock+=c;
                                }
        }
        if (cont) barf("Unexpected end of input.");
        return startNode;
    }
}
xtpAttribute * doParam(string paramName, std::istream xin)
{
    int cont=1;
    xtpAttribute * tempNode;
    xtpNode * startNode;
    xtpNode * thisNode;
    startNode = NULL;
    thisNode=NULL;
    char c;
    string value;
    value="";
    int started=0;
    int isQ=0;
    while((!xin.eof())&&(cont)) {
        xin.get(c);
        if (xin.eof()) break;
        if ((c==' ')||(c=='\n')||(c=='\t')) {
            if ((started)&&(!isQ)) {
                cont=0;
                if (value!="") {
                    if (thisNode != NULL) {
                        thisNode->next=new xtpNode;
                        thisNode=thisNode->next;
                    } else {
                        thisNode=new xtpNode;
                        startNode=thisNode;
                    }
                    thisNode->value=value;
                    thisNode->attributes=NULL;
                    thisNode->next=NULL;
                    value="";
                }
            } else {
                if (started) value+=c;
            }
        } else
            if (c=='>') {
                xin.putback('>');
                if ((started)&&(!isQ)) {
                    cont=0;
                    if (value!="") {
                        if (thisNode != NULL) {
                            thisNode->next=new xtpNode;
                            thisNode=thisNode->next;
                        } else {
                            thisNode=new xtpNode;
                            startNode=thisNode;
                        }
                        thisNode->value=value;
                        thisNode->attributes=NULL;
                        thisNode->next=NULL;
                        value="";
                    }
                } else
                    if (isQ) {
                        value+=c;
                    } else barf("Blank value on attribute again...");
            } else
                if (c=='"') {
                    if ((!isQ)&&(!started)) {
                        started=1;
                        isQ=1;
                    } else
                        if (isQ) {
                            cont=0;
                            if (thisNode != NULL) {
                                thisNode->next=new xtpNode;
                                thisNode=thisNode->next;
                            } else {
                                thisNode=new xtpNode;
                                startNode=thisNode;
                            }
                            thisNode->value=value;
                            thisNode->attributes=NULL;
                            thisNode->next=NULL;
                            value="";
                        } else {
                            //barf("There was a quote out of place in "+paramName+".");
                            if (thisNode!=NULL) delfulltree(startNode);
                            thisNode=new xtpNode;
                            thisNode->attributes=NULL;
                            thisNode->next=NULL;
                            thisNode->value="barf";
                            thisNode->attributes=new xtpAttribute;
                            thisNode->attributes->name="_body";
                            thisNode->attributes->next=NULL;
                            thisNode->attributes->value=new xtpNode;
                            thisNode->attributes->value->attributes=NULL;
                            thisNode->attributes->value->next=NULL;
                            thisNode->attributes->value->value="There was a quote out of place in "+paramName+".";
                            startNode=thisNode;
                        }
                } else 
                    if (c=='\\') {
                        xin.get(c);
                        if (xin.eof()) barf("Escape char (\\) encountered without something to escape at the end of the file.");
                        started=1;
                        switch(c) {
                        case 'n':c='\n';break;
                        case 't':c='\t';break;
                        default:break;
                        }
                        value+=c;
                    } else
                        if (c=='<') {
                            started=1;
                            if (value!="") {
                                if (thisNode != NULL) {
                                    thisNode->next=new xtpNode;
                                    thisNode=thisNode->next;
                                } else {
                                    thisNode=new xtpNode;
                                    startNode=thisNode;
                                }
                                thisNode->value=value;
                                thisNode->attributes=NULL;
                                thisNode->next=NULL;
                                value="";
                            }
                            int cont2=1;
                            while((!xin.eof())&&(cont2)) {
                                xin.get(c);
                                if (xin.eof()) break;
                                if (c=='/') {
                                    barf(paramName+":Closing tags badly nested, ended up in attribute");
                                } else
                                    if (c=='>') {
                                        xin.putback('>');
                                        cont2=0;
                                    }
                                    if ((c==' ')||(c=='\n')||(c=='\t')) {
                                        cont2=0;
                                    } else value+=c;
                            }
                            if (cont2) barf("Unexpected end of input.");
                            if (thisNode != NULL) {
                                thisNode->next=new xtpNode;
                                thisNode=thisNode->next;
                            } else {
                                thisNode=new xtpNode;
                                startNode=thisNode;
                            }
                            thisNode->value=value;
                            thisNode->attributes=doChildren(value,xin);
                            thisNode->next=NULL;
                            value="";
                        } else {
                            value+=c;
                            started=1;
                        }
    }
    if (cont) barf("Unexpected end of input.");
    if (thisNode==NULL) startNode=NULL;
    tempNode=new xtpAttribute;
    tempNode->name=paramName;
    tempNode->next=NULL;
    tempNode->value=startNode;
    return tempNode;
}
void barf(string junk)
{
    cout<<"Expires:0\nContent-Type:Text/Html\nPragma:no-cache\nCache-Control:no-cache\n\n<h5>"<<junk<<"</h5><hr />";
    exit(0);
}
xtpNode * execute(xtpNode * root,xtpAttribute * functions,xtpAttribute * environ)
{
    xtpNode * tempNode;
    xtpAttribute * tempNode2;
    xtpNode * prevNode;
    xtpNode * tempNode3;
    xtpNode * tempNode4;
	xtpNode * tempNode5;
	xtpNode * tempNode6;
	xtpNode * tempNode7;
    LPWIN32_FIND_DATA filedat;
    HANDLE hand;
    string s;
    string strTemp;
    string strTemp2;
    string strTemp3;
    char ctemp;
    char c;
    char x[18];
	long tempInt;
    long tempInt2;
    std::ifstream ifs;
    std::ofstream ofs;
    
    tempNode=root;
    prevNode = NULL;
    int goNext=1;
    while(tempNode != NULL)
    {
		//cout<<(long)tempNode<<'\n';
        if (tempNode->attributes != NULL)
        {
            if ((tempNode->value != "function") && (tempNode->value != "true") && (tempNode->value != "false")) {
                tempNode2=tempNode->attributes;
                while(tempNode2) {
                    s=tempNode2->name;
                    if((s!="function")&&(tempNode2->value != NULL)) tempNode2->value = execute (tempNode2->value,functions,environ);
                    tempNode2=tempNode2->next;
                }
            }
            s=tempNode->value;
            //cout<<s<<" ";
            if (s=="!--") {
                root=delNode(root,prevNode);
                tempNode=tempNode->next;
                goNext=0;
            } else if (s=="param") {
                barf("PARAM DETECTED after VarReplace was called.  ROOT value currently is "+root->value);
            } else if ((s=="true") || (s=="false")) {
            } else if (s=="function") {
                addFunction(functions,tempNode);
            } else if (s=="equal") {
                //get attributes "left" and "right"
                //compare them
                //replace this node with either the True or False subnode within Body
                tempNode2=tempNode->attributes;
                tempNode3=NULL; //body
                while(tempNode2!=NULL)
                {
                    if(tempNode2->name=="_body") {
                        if (tempNode2->value==NULL) barf("no NULL _body in <equal> allowed");
                        tempNode3=tempNode2->value;
                    } else if (tempNode2->name == "left") {
                        if (tempNode2->value==NULL) barf("no NULL left in <equal> allowed");
                        strTemp="";
                        tempNode4=tempNode2->value;
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name == "right") {
                        if (tempNode2->value==NULL) barf("no NULL right in <equal> allowed");
                        strTemp2="";
                        tempNode4=tempNode2->value;
                        while(tempNode4!=NULL) {
                            strTemp2+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempNode3==NULL) barf("No _body in an <equal>");
                tempNode4=NULL;
                while(tempNode3!=NULL)
                {
                    if ((tempNode3->value=="true") && (strTemp==strTemp2)) {
                        tempNode2=tempNode3->attributes;
                        tempNode4=NULL;
                        while(tempNode2!=NULL)
                        {
                            if (tempNode2->name=="_body") {
                                if (tempNode2->value==NULL) barf("no NULL _body in <true> allowed");
                                tempNode4=tempNode2->value;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (tempNode4==NULL) barf("No _body in a <true>");
                        root=insFunc(root,prevNode,tempNode4,NULL);
						deltree(tempNode);
						break;
                    }
                    if ((tempNode3->value=="false") && (strTemp!=strTemp2)) {
                        tempNode2=tempNode3->attributes;
                        tempNode4=NULL;
                        while(tempNode2!=NULL)
                        {
                            if (tempNode2->name=="_body") {
                                if (tempNode2->value==NULL) barf("no NULL _body in <false> allowed");
                                tempNode4=tempNode2->value;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (tempNode4==NULL) barf("No _body in a <false>");
                        root=insFunc(root,prevNode,tempNode4,NULL);
						deltree(tempNode);
						break;
                    }
                    tempNode3=tempNode3->next;
                }
                if (tempNode4!=NULL) {
                    if (prevNode!=NULL)
                        tempNode=prevNode->next;
                    else 
                        tempNode=root;
                    goNext=0;
                } else goNext=1;
            } else if (s=="like") {
                //get attributes "left" and "right"
                //compare them
                //replace this node with either the True or False subnode within Body
                tempNode2=tempNode->attributes;
                tempNode3=NULL; //body
                while(tempNode2!=NULL)
                {
                    if(tempNode2->name=="_body") {
                        if (tempNode2->value==NULL) barf("no NULL _body in <like> allowed");
                        tempNode3=tempNode2->value;
                    } else if (tempNode2->name == "left") {
                        if (tempNode2->value==NULL) barf("no NULL left in <like> allowed");
                        strTemp="";
                        tempNode4=tempNode2->value;
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name == "right") {
                        if (tempNode2->value==NULL) barf("no NULL right in <like> allowed");
                        strTemp2="";
                        tempNode4=tempNode2->value;
                        while(tempNode4!=NULL) {
                            strTemp2+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempNode3==NULL) barf("No _body in an <like>");
                tempNode4=NULL;
                while(tempNode3!=NULL)
                {
                    if ((tempNode3->value=="true") && (regmatch(strTemp2,strTemp))) {
                        tempNode2=tempNode3->attributes;
                        tempNode4=NULL;
                        while(tempNode2!=NULL)
                        {
                            if (tempNode2->name=="_body") {
                                if (tempNode2->value==NULL) barf("no NULL _body in <true> allowed");
                                tempNode4=tempNode2->value;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (tempNode4==NULL) barf("No _body in a <true>");
                        root=insFunc(root,prevNode,tempNode4,NULL);
						deltree(tempNode);
						break;
                    }
                    if ((tempNode3->value=="false") && (!regmatch(strTemp2,strTemp))) {
                        tempNode2=tempNode3->attributes;
                        tempNode4=NULL;
                        while(tempNode2!=NULL)
                        {
                            if (tempNode2->name=="_body") {
                                if (tempNode2->value==NULL) barf("no NULL _body in <false> allowed");
                                tempNode4=tempNode2->value;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (tempNode4==NULL) barf("No _body in a <false>");
                        root=insFunc(root,prevNode,tempNode4,NULL);
						deltree(tempNode);
						break;
                    }
                    tempNode3=tempNode3->next;
                }
                if (tempNode4!=NULL) {
                    if (prevNode!=NULL)
                        tempNode=prevNode->next;
                    else 
                        tempNode=root;
                    goNext=0;
                } else goNext=1;
            } else if (s=="paramExists") {
                //get attributes "left" and "right"
                //compare them
                //replace this node with either the True or False subnode within Body
                tempNode2=tempNode->attributes;
                tempNode3=NULL; //body
                while(tempNode2!=NULL)
                {
                    if(tempNode2->name=="_body") {
                        if (tempNode2->value==NULL) barf("no NULL _body in <paramExists> allowed");
                        tempNode3=tempNode2->value;
                    } else if (tempNode2->name == "_name") {
                        if (tempNode2->value==NULL) barf("no NULL _name in <paramExists> allowed");
                        strTemp="";
                        tempNode4=tempNode2->value;
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempNode3==NULL) barf("No _body in a <paramExists>");
                tempNode4=NULL;
                tempNode2=environ;
                tempInt=0;
                while(tempNode2!=NULL) {
                    if (tempNode2->name==strTemp) {
                        tempInt=1;
                        break;
                    }
                }
                while(tempNode3!=NULL)
                {
                    if ((tempNode3->value=="true") && (tempInt==1)) {
                        tempNode2=tempNode3->attributes;
                        tempNode4=NULL;
                        while(tempNode2!=NULL)
                        {
                            if (tempNode2->name=="_body") {
                                if (tempNode2->value==NULL) barf("no NULL _body in <true> allowed");
                                tempNode4=tempNode2->value;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (tempNode4==NULL) barf("No _body in a <true>");
                        root=insFunc(root,prevNode,tempNode4,NULL);
						deltree(tempNode);
						break;
                    }
                    if ((tempNode3->value=="false") && (tempInt==0)) {
                        tempNode2=tempNode3->attributes;
                        tempNode4=NULL;
                        while(tempNode2!=NULL)
                        {
                            if (tempNode2->name=="_body") {
                                if (tempNode2->value==NULL) barf("no NULL _body in <false> allowed");
                                tempNode4=tempNode2->value;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (tempNode4==NULL) barf("No _body in a <false>");
                        root=insFunc(root,prevNode,tempNode4,NULL);
						deltree(tempNode);
						break;
                    }
                    tempNode3=tempNode3->next;
                }
                if (tempNode4!=NULL) {
                    if (prevNode!=NULL)
                        tempNode=prevNode->next;
                    else 
                        tempNode=root;
                    goNext=0;
                } else goNext=1;
            } else if (s=="varExists") {
                //get attributes "left" and "right"
                //compare them
                //replace this node with either the True or False subnode within Body
                tempNode2=tempNode->attributes;
                tempNode3=NULL; //body
                while(tempNode2!=NULL)
                {
                    if(tempNode2->name=="_body") {
                        if (tempNode2->value==NULL) barf("no NULL _body in <varExists> allowed");
                        tempNode3=tempNode2->value;
                    } else if (tempNode2->name == "_name") {
                        if (tempNode2->value==NULL) barf("no NULL _name in <varExists> allowed");
                        strTemp="";
                        tempNode4=tempNode2->value;
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempNode3==NULL) barf("No _body in a <varExists>");
                tempNode4=NULL;
                tempNode2=vars;
                tempInt=0;
                while(tempNode2!=NULL) {
                    if (tempNode2->name==strTemp) {
                        tempInt=1;
                        break;
                    }
                }
                while(tempNode3!=NULL)
                {
                    if ((tempNode3->value=="true") && (tempInt==1)) {
                        tempNode2=tempNode3->attributes;
                        tempNode4=NULL;
                        while(tempNode2!=NULL)
                        {
                            if (tempNode2->name=="_body") {
                                if (tempNode2->value==NULL) barf("no NULL _body in <true> allowed");
                                tempNode4=tempNode2->value;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (tempNode4==NULL) barf("No _body in a <true>");
                        root=insFunc(root,prevNode,tempNode4,NULL);
						deltree(tempNode);
						break;
                    }
                    if ((tempNode3->value=="false") && (tempInt==0)) {
                        tempNode2=tempNode3->attributes;
                        tempNode4=NULL;
                        while(tempNode2!=NULL)
                        {
                            if (tempNode2->name=="_body") {
                                if (tempNode2->value==NULL) barf("no NULL _body in <false> allowed");
                                tempNode4=tempNode2->value;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (tempNode4==NULL) barf("No _body in a <false>");
                        root=insFunc(root,prevNode,tempNode4,NULL);
						deltree(tempNode);
						break;
                    }
                    tempNode3=tempNode3->next;
                }
                if (tempNode4!=NULL) {
                    if (prevNode!=NULL)
                        tempNode=prevNode->next;
                    else 
                        tempNode=root;
                    goNext=0;
                } else goNext=1;
            } else if (s=="pretty") {
				pretty=true;
                tempNode->attributes=NULL;
                tempNode->value="";
            } else if (s=="log") {
                tempNode2=tempNode->attributes;
                //parameters:mode (allBefore/allAfter),find,_body
                strTemp="";
                strTemp2="";
                strTemp3="";
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in log cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name=="file") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("file in log cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp2+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                ofs.clear();
                strTemp2="xtp."+strTemp2+".log";
                ofs.open(strTemp2.c_str(),std::ofstream::app | std::ofstream::out);
                if (!ofs.fail()) {
                    ofs<<strTemp;
                    ofs.close();
                } else {
                    barf("bad file in log.");
                }
				blanktree(tempNode);
                /*tempNode->attributes=NULL;
                tempNode->value="";*/
                //cout<<strTemp<<" "<<strTemp2<<" "<<x;
            } else if (s=="dec") {
                tempNode2=tempNode->attributes;
                //parameters:mode (allBefore/allAfter),find,_body
                strTemp="";
                strTemp2="";
                strTemp3="";
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in dec cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name=="by") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("by in dec cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp2+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                if (strTemp2=="") strTemp2="1";
                //x = "                 ";
                itoa(atoi(strTemp.c_str())-atoi(strTemp2.c_str()),x,10);
                strTemp3=x;
                blanktree(tempNode);
                tempNode->value=strTemp3;
                //cout<<strTemp<<" "<<strTemp2<<" "<<x;
            } else if (s=="inc") {
                tempNode2=tempNode->attributes;
                //parameters:mode (allBefore/allAfter),find,_body
                strTemp="";
                strTemp2="";
                strTemp3="";
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in dec cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name=="by") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("by in dec cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp2+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                if (strTemp2=="") strTemp2="1";
                itoa(atoi(strTemp.c_str())+atoi(strTemp2.c_str()),x,10);
                strTemp3=x;
                blanktree(tempNode);
                tempNode->value=strTemp3;
            } else if (s=="setVar") {
                tempNode2=tempNode->attributes;
                //parameters:mode (allBefore/allAfter),find,_body
                strTemp="";
                strTemp2="";
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "_name") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_name in setVar cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp2+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in setVar cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                tempNode2=new xtpAttribute;
                tempNode2->name=strTemp2;
                tempNode2->value=new xtpNode;
                tempNode2->value->next=NULL;
                tempNode2->value->attributes=NULL;
                tempNode2->value->value=strTemp;
                tempNode2->next=vars;
                vars=tempNode2;
                blanktree(tempNode);
            } else if (s=="getVar") {
                tempNode2=tempNode->attributes;
                //parameters:mode (allBefore/allAfter),find,_body
                strTemp="";
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "_name") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_name in getVar cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                tempNode2=vars;
                tempInt=0;
                while(tempNode2!=NULL) {
                    if (tempNode2->name == strTemp) {
		                blanktree(tempNode);
                        tempNode->value=tempNode2->value->value;
                        tempInt=1;
                        break;
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempInt==0) {
	                blanktree(tempNode);
                }
            } else if (s=="stringChunk") {
                tempNode2=tempNode->attributes;
                //parameters:mode (allBefore/allAfter),find,_body
                strTemp="";
                strTemp2="";
                strTemp3="";
                tempInt=0;
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "mode") {
                        if (tempNode2->value == NULL) barf("mode in stringChunk cannot be NULL");
                        tempNode4=tempNode2->value;
                        while(tempNode4!=NULL) {
                            strTemp2+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name == "find") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("find in stringChunk cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp3+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in stringChunk cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    } else if (tempNode2->name == "reverse") {
                        tempInt=1;
                    }
                    tempNode2=tempNode2->next;
                }
                blanktree(tempNode);
				//tempNode->attributes=NULL;
                if (tempInt==0) {
                    tempInt=strTemp.find(strTemp3);
                } else {
                    tempInt=strTemp.find(strTemp3);
                    if (tempInt>=0) {
                        tempInt2=tempInt;
                        while (tempInt2>=0) {
                            tempInt=tempInt2;
                            tempInt2=strTemp.find(strTemp3,tempInt+1);
                        }
                    }
                }
                if (strTemp2=="allBefore") {
                    if (tempInt>=0) {
                        strTemp=strTemp.substr(0,tempInt);
                    } else {
                        strTemp="";
                    }
                } else if (strTemp2=="allAfter") {
                    if (tempInt>=0) {
                        if ((tempInt+1)<strTemp.length()) {
                            strTemp=strTemp.substr(tempInt+strTemp3.length(),(strTemp.length()-tempInt)-strTemp3.length());
                        } else {
                            strTemp="";
                        }
                    } else {
                        strTemp="";
                    }
                } else {
                    strTemp="";
                }
                tempNode->value=strTemp;
            } else if (s=="unlink") {
                tempNode2=tempNode->attributes;
                strTemp="";
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in unlink cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                strTemp=deScape(strTemp);
                unlink(strTemp.c_str());
                blanktree(tempNode);
                tempNode->value="";
            } else if (s=="descape") {
                tempNode2=tempNode->attributes;
                strTemp="";
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in descape cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                strTemp=deScape(strTemp);
                blanktree(tempNode);
                tempNode->value=strTemp;
            } else if (s=="escape") {
                tempNode2=tempNode->attributes;
                strTemp="";
                while(tempNode2!=NULL) {
                    if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in escape cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    tempNode2=tempNode2->next;
                }
                strTemp=eScape(strTemp);
                blanktree(tempNode);
                tempNode->value=strTemp;
            } else if (s=="random") {
				tempNode3=NULL;
				tempNode2=tempNode->attributes;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in random cannot be NULL");
						tempNode3=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				strTemp="";
				while (tempNode3 !=NULL) {
					strTemp+=outputNode(tempNode3);
					tempNode3=tempNode3->next;
				}
				if (strTemp!="") {
					itoa(int(rand()*(
						float(atoi(strTemp.c_str()))/RAND_MAX
						)),x,10);
				} else {
					itoa(rand(),x,10);
				}
                strTemp=x;
                blanktree(tempNode);
                tempNode->value=strTemp;
			} else if (s=="dbSelect") {
                strTemp="";
				strTemp2="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				tempNode4=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in dbSelect cannot be NULL");
						tempNode3=tempNode2->value;
					} else if (tempNode2->name=="onError") {
						if (tempNode2->value==NULL) barf ("onError in dbSelect cannot be NULL");
						tempNode4=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad dbSelect");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				if (tempNode4==NULL) {
					strTemp2="dbError";
				} else {
					while(tempNode4!=NULL)
					{
						strTemp2+=outputNode(tempNode4);
						tempNode4=tempNode4->next;
					}
				}
				tempNode7=dbFunc(strTemp2,selectDB(strTemp));
				root=insFunc(root,prevNode,tempNode7,NULL);
				delfulltree(tempNode7);
				deltree(tempNode);
                if (prevNode!=NULL)
                    tempNode=prevNode->next;
                else 
                    tempNode=root;
			} else if (s=="dbDelete") {
                strTemp="";
				strTemp2="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				tempNode4=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in dbDelete cannot be NULL");
						tempNode3=tempNode2->value;
					} else if (tempNode2->name=="onError") {
						if (tempNode2->value==NULL) barf ("onError in dbDelete cannot be NULL");
						tempNode4=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad dbDelete");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				if (tempNode4==NULL) {
					strTemp2="dbError";
				} else {
					while(tempNode4!=NULL)
					{
						strTemp2+=outputNode(tempNode4);
						tempNode4=tempNode4->next;
					}
				}
				tempNode7=dbFunc(strTemp2,rmRec(strTemp));
				root=insFunc(root,prevNode,tempNode7,NULL);
				delfulltree(tempNode7);
				deltree(tempNode);
                if (prevNode!=NULL)
                    tempNode=prevNode->next;
                else 
                    tempNode=root;
			} else if (s=="hash") {
                strTemp="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in hash cannot be NULL");
						tempNode3=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad hash");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				blanktree(tempNode);
				tempNode->value=hash(strTemp);
            } else if (s=="fileCopy") {
                //to defines the target
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
                strTemp="";
                strTemp2="";
				while (tempNode2!=NULL) {
                    if (tempNode2->name == "_body") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("_body in fileCopy cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
                    if (tempNode2->name == "to") {
                        tempNode4=tempNode2->value;
                        if (tempNode2->value == NULL) barf("to in fileCopy cannot be NULL");
                        while(tempNode4!=NULL) {
                            strTemp2+=outputNode(tempNode4);
                            tempNode4=tempNode4->next;
                        }
                    }
					tempNode2=tempNode2->next;
				}
                std::ifstream inew;
                std::ofstream onew;
                inew.open(strTemp.c_str(),std::ios::binary);
                onew.open(strTemp2.c_str(),std::ios::binary);
                if (inew.fail()) barf("Bad input filename");
                if (onew.fail()) barf("Bad output filename");
                if ((inew.is_open()) && (onew.is_open())) {
                    while (!inew.eof())
                    {
                        ctemp=inew.get();
                        if (inew.eof()) break;
                        onew.put(ctemp);
                    }
                }
                inew.close();
                onew.close();
				blanktree(tempNode);
                tempNode->value="";
            } else if (s=="fileReadOpen") {
                strTemp="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in fileReadOpen cannot be NULL");
						tempNode3=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad fileReadOpen");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				blanktree(tempNode);
                if (xread.is_open()) xread.close();
                xread.open(strTemp.c_str(),std::ios_base::binary);
                if (xread.fail()) barf("Bad filename in fileReadOpen - "+strTemp);
                tempNode->value="";
            } else if (s=="fileReadUntil") {
                strTemp="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
                tempInt=0;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in fileReadUntil cannot be NULL");
						tempNode3=tempNode2->value;
					} else if (tempNode2->name=="escape") {
                        tempInt=1;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad fileReadUntil");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
                if (strTemp.length()>1) barf("fileReadUntil can only readUntil one character");
				strTemp2="";
                blanktree(tempNode);
                if (!xread.is_open()) barf("No File Open in fileReadUntil");
                while (!xread.eof())
                {
                    xread.get(c);
                    if (c=='\\' && tempInt==1) {
                        xread.get(c);
                        strTemp2+=c;
                    } else {
                        if (c!=strTemp[0]) strTemp2+=c; else break;
                    }
                }
                tempNode->value=strTemp2;
            } else if (s=="fileReadChar") {
                strTemp="";
                if (!xread.is_open()) barf("No File Open in fileReadChar");
                blanktree(tempNode);
                if(!xread.eof())
                {
                    xread.get(c);
                }
                strTemp+=c;
                tempNode->value=strTemp;
            } else if (s=="fileReadEOF") {
                blanktree(tempNode);
                if (!xread.is_open()) barf("No file open in fileReadEOF");
                if (xread.eof()) tempNode->value="TRUE"; else tempNode->value="FALSE";
            } else if (s=="fileReadClose") {
                xread.close();
                xread.fail();
                blanktree(tempNode);
                tempNode->value="";
			} else if (s=="fileWriteOpen") {
                strTemp="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in fileWriteOpen cannot be NULL");
						tempNode3=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad fileWriteOpen");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				blanktree(tempNode);
                if (xwrite.is_open()) xwrite.close();
                xwrite.open(strTemp.c_str(),std::ios_base::binary);
                if (xwrite.fail()) barf("Bad filename in fileWriteOpen - "+strTemp);
                tempNode->value="";
			} else if (s=="fileWriteAppend") {
                strTemp="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in fileWriteAppend cannot be NULL");
						tempNode3=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad fileWriteAppend");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				blanktree(tempNode);
                if (xwrite.is_open()) xwrite.close();
                xwrite.open(strTemp.c_str(),std::ofstream::app | std::ofstream::out | std::ios_base::binary);
                if (xwrite.fail()) barf("Bad filename in fileWriteAppend - "+strTemp);
                tempNode->value="";
            } else if (s=="fileWrite") {
                strTemp="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						//if (tempNode2->value==NULL) strTemp="
						tempNode3=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				//if (tempNode3==NULL) barf("Bad fileWrite");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
                blanktree(tempNode);
                if (!xwrite.is_open()) barf("No File Open in fileReadUntil");
                xwrite.write(strTemp.c_str(),strTemp.length());
                tempNode->value="";
            } else if (s=="fileWriteClose") {
                xwrite.close();
                xwrite.fail();
                blanktree(tempNode);
                tempNode->value="";
			} else if (s=="dbChange") {
                strTemp="";
				strTemp2="";
				strTemp3="";
				s="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				tempNode4=NULL;
				tempNode5=NULL;
				tempNode6=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in dbChange cannot be NULL");
						tempNode3=tempNode2->value;
					} else if (tempNode2->name=="_name") {
						if (tempNode2->value==NULL) barf ("_name in dbChange cannot be NULL");
						tempNode5=tempNode2->value;
					} else if (tempNode2->name=="_recordID") {
						if (tempNode2->value==NULL) barf ("_recordID in dbChange cannot be NULL");
						tempNode6=tempNode2->value;
					} else if (tempNode2->name=="onError") {
						if (tempNode2->value==NULL) barf ("onError in dbChange cannot be NULL");
						tempNode4=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad dbChange - no body");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				if (tempNode5==NULL) barf("Bad dbChange - no _name");
                while(tempNode5!=NULL)
                {
                    strTemp3+=outputNode(tempNode5);
                    tempNode5=tempNode5->next;
                }
				if (tempNode6==NULL) barf("Bad dbChange - no _recordID");
                while(tempNode6!=NULL)
                {
                    s+=outputNode(tempNode6);
                    tempNode6=tempNode6->next;
                }
				if (tempNode4==NULL) {
					strTemp2="dbError";
				} else {
					while(tempNode4!=NULL)
					{
						strTemp2+=outputNode(tempNode4);
						tempNode4=tempNode4->next;
					}
				}
				tempNode7=dbFunc(strTemp2,updateDB(s,strTemp3,strTemp));
				root=insFunc(root,prevNode,tempNode7,NULL);
				delfulltree(tempNode7);
				deltree(tempNode);
                if (prevNode!=NULL)
                    tempNode=prevNode->next;
                else 
                    tempNode=root;
			} else if (s=="dbQuery") {
                strTemp="";
				strTemp2="";
				strTemp3="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				tempNode4=NULL;
				tempNode5=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in dbQuery cannot be NULL");
						tempNode3=tempNode2->value;
					} else if (tempNode2->name=="callback") {
						if (tempNode2->value==NULL) barf ("callback in dbQuery cannot be NULL");
						tempNode5=tempNode2->value;
					} else if (tempNode2->name=="onError") {
						if (tempNode2->value==NULL) barf ("onError in dbQuery cannot be NULL");
						tempNode4=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad dbQuery - no _body");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				if (tempNode5==NULL) barf("Bad dbQuery - no callback");
                while(tempNode5!=NULL)
                {
                    strTemp3+=outputNode(tempNode5);
                    tempNode5=tempNode5->next;
                }
				if (tempNode4==NULL) {
					strTemp2="dbError";
				} else {
					while(tempNode4!=NULL)
					{
						strTemp2+=outputNode(tempNode4);
						tempNode4=tempNode4->next;
					}
				}
				tempNode7=queryResults(strTemp3,strTemp2,queryDB(strTemp));
				root=insFunc(root,prevNode,tempNode7,NULL);
				delfulltree(tempNode7);
				deltree(tempNode);
                if (prevNode!=NULL)
                    tempNode=prevNode->next;
                else 
                    tempNode=root;
			} else if (s=="dbQueryCount") {
                strTemp="";
				strTemp2="";
				strTemp3="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				tempNode4=NULL;
				tempNode5=NULL;
				while (tempNode2!=NULL) {
					if(tempNode2->name=="_body") {
						if (tempNode2->value==NULL) barf ("_body in dbQuery cannot be NULL");
						tempNode3=tempNode2->value;
					} else if (tempNode2->name=="onError") {
						if (tempNode2->value==NULL) barf ("onError in dbQuery cannot be NULL");
						tempNode4=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode3==NULL) barf("Bad dbQuery - no _body");
                while(tempNode3!=NULL)
                {
                    strTemp+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
				if (tempNode4==NULL) {
					strTemp2="dbError";
				} else {
					while(tempNode4!=NULL)
					{
						strTemp2+=outputNode(tempNode4);
						tempNode4=tempNode4->next;
					}
				}
				tempNode7=dbIDFunc(strTemp2,queryDBCount(strTemp));
				root=insFunc(root,prevNode,tempNode7,NULL);
				delfulltree(tempNode7);
				deltree(tempNode);
                if (prevNode!=NULL)
                    tempNode=prevNode->next;
                else 
                    tempNode=root;
			} else if (s=="dbCreate") {
				strTemp2="";
				//get the database name (_body) and error function if present.
				tempNode2=tempNode->attributes;
				tempNode3=NULL;
				tempNode4=NULL;
				while (tempNode2!=NULL) {
					if (tempNode2->name=="onError") {
						if (tempNode2->value==NULL) barf ("onError in selectDB cannot be NULL");
						tempNode4=tempNode2->value;
					}
					tempNode2=tempNode2->next;
				}
				if (tempNode4==NULL) {
					strTemp2="dbError";
				} else {
					while(tempNode4!=NULL)
					{
						strTemp2+=outputNode(tempNode4);
						tempNode4=tempNode4->next;
					}
				}
				tempNode7=dbIDFunc(strTemp2,dbCreate());
				root=insFunc(root,prevNode,tempNode7,NULL);
				delfulltree(tempNode7);
				deltree(tempNode);
                if (prevNode!=NULL)
                    tempNode=prevNode->next;
                else 
                    tempNode=root;
            } else if (s=="filelist") { //filelist always parents the list in a <flresult> tag.
                tempNode2=tempNode->attributes;
                tempNode3=NULL;
                tempNode5=NULL;
                while(tempNode2!=NULL) {
                    if(tempNode2->name=="_body") {
                        if (tempNode2->value == NULL) barf("_body in filelist cannot be NULL");
                        tempNode3=tempNode2->value;
					} else if (tempNode2->name=="callback") {
						if (tempNode2->value==NULL) barf ("callback in filelist cannot be NULL");
						tempNode5=tempNode2->value;
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempNode3==NULL) {
                    blanktree(tempNode);
                    tempNode->value="ferr";
                    tempNode->attributes=new xtpAttribute;
                    tempNode->attributes->name="_body";
                    tempNode->attributes->next=NULL;
                    tempNode->attributes->value=new xtpNode;
                    tempNode->attributes->value->attributes=NULL;
                    tempNode->attributes->value->next=NULL;
                    tempNode->attributes->value->value="Bad filelist.";
                    goNext=0;
                } else {
                    s="";
                    while(tempNode3!=NULL)
                    {
                        s+=outputNode(tempNode3);
                        tempNode3=tempNode3->next;
                    }
                    strTemp="";
                    while(tempNode5!=NULL)
                    {
                        strTemp+=outputNode(tempNode5);
                        tempNode5=tempNode5->next;
                    }
                    blanktree(tempNode);

                    filedat=new WIN32_FIND_DATA;
                    hand=FindFirstFile(s.c_str(),filedat);
                    //INVALID_HANDLE_VALUE
                    if (hand!=INVALID_HANDLE_VALUE) {
                        tempNode->value="flresult";
                        tempNode->attributes=new xtpAttribute;
                        tempNode->attributes->name="_body";
                        tempNode->attributes->next=NULL;
                        tempNode4=NULL;
                        tempInt=true;
                        while (tempInt) {
                            tempNode6=new xtpNode;
                            tempNode6->value=strTemp;
                            tempNode6->attributes=new xtpAttribute;
                            tempNode6->next=NULL;
                            tempNode2=tempNode6->attributes;
                            tempNode2->name="_body";
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            tempNode2->value->value=filedat->cFileName;

                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filearchive";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filecompressed";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filefolder";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="fileencrypted";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filehidden";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="fileoffline";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filereadonly";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filereparse";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filesparse";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filesystem";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            tempNode2->next=new xtpAttribute;
                            tempNode2=tempNode2->next;
                            tempNode2->name="filetemporary";
                            tempNode2->next=NULL;
                            tempNode2->value=new xtpNode;
                            tempNode2->value->attributes=NULL;
                            tempNode2->value->next=NULL;
                            if (filedat->dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)
                                tempNode2->value->value="true";
                            else
                                tempNode2->value->value="false";
                            if (tempNode4==NULL) {
                                tempNode->attributes->value=tempNode6;
                                tempNode4=tempNode6;
                            } else {
                                tempNode4->next=tempNode6;
                                tempNode4=tempNode4->next;
                            }
                            tempInt=FindNextFile(hand,filedat);
                        }
                        FindClose(hand);
                        goNext=0;
                    } else {
                        tempNode->value="ferr";
                        tempNode->attributes=new xtpAttribute;
                        tempNode->attributes->name="_body";
                        tempNode->attributes->next=NULL;
                        tempNode->attributes->value=new xtpNode;
                        tempNode->attributes->value->attributes=NULL;
                        tempNode->attributes->value->next=NULL;
                        tempNode->attributes->value->value="Empty filelist.";
                        goNext=0;
                    }
                }                
            } else if (s=="include") {
                tempNode2=tempNode->attributes;
                tempNode3=NULL;
                tempInt=0;
                while(tempNode2!=NULL) {
                    if(tempNode2->name=="_body") {
                        if (tempNode2->value == NULL) barf("_body in include cannot be NULL");
                        tempNode3=tempNode2->value;
                    } else if (tempNode2->name=="noparse") {
                        tempInt=1;
                    } else if (tempNode2->name=="noexec") {
                        tempInt=-1;
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempNode3==NULL) {
                    blanktree(tempNode);
                    tempNode->value="ibarf";
                    tempNode->attributes=new xtpAttribute;
                    tempNode->attributes->name="_body";
                    tempNode->attributes->next=NULL;
                    tempNode->attributes->value=new xtpNode;
                    tempNode->attributes->value->attributes=NULL;
                    tempNode->attributes->value->next=NULL;
                    tempNode->attributes->value->value="Bad include.";
                    goNext=0;
                    //barf("Bad include."); 
                } else {
                    s="";
                    while(tempNode3!=NULL)
                    {
                        s+=outputNode(tempNode3);
                        tempNode3=tempNode3->next;
                    }
                    ifs.open(s.c_str());
                    if (ifs.fail()) {
                        blanktree(tempNode);
                        tempNode->value="ibarf";
                        tempNode->attributes=new xtpAttribute;
                        tempNode->attributes->name="_body";
                        tempNode->attributes->next=NULL;
                        tempNode->attributes->value=new xtpNode;
                        tempNode->attributes->value->attributes=NULL;
                        tempNode->attributes->value->next=NULL;
                        tempNode->attributes->value->value="Bad include:"+s;
                        goNext=0;
                        //barf("BAD INCLUDE:"+s);
                    } else {
				        if (tempInt==-1) {
					        tempNode5=tagify("",ifs);
					        tempNode6=tempNode5;
					        s="";
					        while(tempNode5!=NULL) {
						        s+=outputNode3(tempNode5,0);
						        tempNode5=tempNode5->next;
					        }
					        s=htescape(s);
					        delfulltree(tempNode5);
                            tempNode3=new xtpNode;
                            tempNode3->attributes=NULL;
                            tempNode3->next=NULL;
                            tempNode3->value=s;
                            root=insFunc(root,prevNode,tempNode3,NULL);
					        deltree(tempNode3);
				        } else if (tempInt==0) {
					        tempNode5=tagify("",ifs);
                            root=insFunc(root,prevNode,tempNode5,NULL);
					        delfulltree(tempNode5);
                        } else {
                            tempNode3=new xtpNode;
                            tempNode3->attributes=NULL;
                            tempNode3->next=NULL;
                            s="";
                            ifs.get(c);
                            while(!ifs.eof()) {
                                s+=c;
                                ifs.get(c);
                            }
                            tempNode3->value=s;
                            root=insFunc(root,prevNode,tempNode3,NULL);
					        deltree(tempNode3);
                        }
                        goNext=0;
                        ifs.close();
				        deltree(tempNode);
                        if (prevNode!=NULL)
                            tempNode=prevNode->next;
                        else 
                            tempNode=root;
                        //tempNode=tempNode->next;
                    }
                }
            } else if (s=="winclude") {
                tempNode2=tempNode->attributes;
                tempNode3=NULL;
                tempInt=0;
                while(tempNode2!=NULL) {
                    if(tempNode2->name=="_body") {
                        if (tempNode2->value == NULL) barf("_body in include cannot be NULL");
                        tempNode3=tempNode2->value;
                    } else if (tempNode2->name=="noparse") {
                        tempInt=1;
                    } else if (tempNode2->name=="noexec") {
                        tempInt=-1;
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempNode3==NULL) barf("Bad include.");
                s="";
                while(tempNode3!=NULL)
                {
                    s+=outputNode(tempNode3);
                    tempNode3=tempNode3->next;
                }
                //ifs.open(s.c_str());
                //if (ifs.fail()) barf("BAD INCLUDE:"+s);
                strTemp3=sockGet(s);
                if (strTemp3[0]=='<') {
                    blanktree(tempNode);
                    tempNode->value=strTemp3;
                } else {
                    ifs.open(strTemp3.c_str());
				    if (tempInt==-1) {
					    tempNode5=tagify("",ifs);
					    tempNode6=tempNode5;
					    s="";
					    while(tempNode5!=NULL) {
						    s+=outputNode3(tempNode5,0);
						    tempNode5=tempNode5->next;
					    }
					    s=htescape(s);
					    delfulltree(tempNode5);
                        tempNode3=new xtpNode;
                        tempNode3->attributes=NULL;
                        tempNode3->next=NULL;
                        tempNode3->value=s;
                        root=insFunc(root,prevNode,tempNode3,NULL);
					    deltree(tempNode3);
				    } else if (tempInt==0) {
					    tempNode5=tagify("",ifs);
                        root=insFunc(root,prevNode,tempNode5,NULL);
					    delfulltree(tempNode5);
                    } else {
                        tempNode3=new xtpNode;
                        tempNode3->attributes=NULL;
                        tempNode3->next=NULL;
                        s="";
                        while(!ifs.eof()) {
                            ifs.get(c);
                            s+=c;
                        }
                        tempNode3->value=s;
                        root=insFunc(root,prevNode,tempNode3,NULL);
					    deltree(tempNode3);
                    }
                    goNext=0;
                    ifs.close();
                    unlink(strTemp3.c_str());
				    deltree(tempNode);
                    if (prevNode!=NULL)
                        tempNode=prevNode->next;
                    else 
                        tempNode=root;
                }
                //tempNode=tempNode->next;
            } else {
                tempNode3=findFunction(functions,s);
                if (tempNode3!=NULL) {
                    attatch(tempNode->attributes,environ);
                    root=insFunc(root,prevNode,tempNode3,tempNode->attributes);
					deltree(tempNode);
                    if (prevNode!=NULL)
                        tempNode=prevNode->next;
                    else 
                        tempNode=root;
                    goNext=0;
                } else {
                    //tempNode->value=outputNode(tempNode);
                    //tempNode->attributes=NULL;
                    goNext=1;
                }
            }
        } else {
            goNext=1;
        }
        if (goNext) 
        {
            prevNode=tempNode;
            tempNode=tempNode->next;
        } else goNext=1;
    }
    return root;
}
void attatch(xtpAttribute * onto,xtpAttribute * what)
{
    xtpAttribute * temp;
    temp=onto;
    if (temp==NULL) barf("somehow onto in attatch is NULL");
    while (temp->next!=NULL)
        temp=temp->next;
	while (what!=NULL) {
		temp->next=new xtpAttribute;
		temp->next->name=what->name;
		copyNodes(temp->next,what);
		temp->next->next=NULL;
		temp=temp->next;
		what=what->next;
	}
    //temp->next=what;
}
string outputNode(xtpNode * nodeToOutput)
{
    xtpAttribute * tempNode;
    xtpNode * tempNode2;
    xtpNode * bodyVal;
    string str;
    str="";
    bodyVal=NULL;
    tempNode=nodeToOutput->attributes;
    if (nodeToOutput->attributes == NULL)
    {
        str=str+nodeToOutput->value;
    } else {
        if ((nodeToOutput->value != "function") && (nodeToOutput->value != "true") && (nodeToOutput->value != "false") && (nodeToOutput->value != "equal")) {
            str=str+"<"+nodeToOutput->value;
            if (nodeToOutput->value=="nopretty") {
                while(tempNode!=NULL) {
                    if (tempNode->name == "_body") {
                        tempNode2=tempNode->value;
                        while(tempNode2!=NULL) {
                            str+=outputNode(tempNode2);
                            tempNode2=tempNode2->next;
                        }
                    }
                    tempNode=tempNode->next;
                }
			} else {
				while(tempNode!=NULL) {
					if (tempNode->name == "_body") {
						bodyVal=tempNode->value;
					} else {
						if (tempNode->value != NULL) {
							str=str+" "+tempNode->name+"=\"";
							tempNode2=tempNode->value;
							while(tempNode2!=NULL) {
								str=str+outputNode(tempNode2);
								tempNode2=tempNode2->next;
							}
							str=str+"\"";
						} else {
							str+=" "+tempNode->name;
						}
					}
					tempNode=tempNode->next;
				}
				if (bodyVal!=NULL) {
					if ((bodyVal->attributes == NULL) && (bodyVal->next == NULL) && (bodyVal->value == "")) {
						str=str+"/>";
					} else {
						str=str+">";
						tempNode2=bodyVal;
						while(tempNode2!=NULL) {
							str=str+outputNode(tempNode2);
							tempNode2=tempNode2->next;
						}
						str=str+"</"+nodeToOutput->value+">";
					}
				} else {
					str=str+"/>";
				}
			}
        }
    }
    return str;
}
void addFunction(xtpAttribute * functions,xtpNode * funcTop)
{
    //find _name and _body attributes.  get the string from name.
    xtpNode * bodyPtr;
    string name;
    xtpAttribute * tempNode;
    tempNode = funcTop->attributes;
    bodyPtr=NULL;
    while (tempNode != NULL) {
        if (tempNode->name=="_name") {
            if (tempNode->value == NULL) barf("Function name cannot be NULL");
            if (tempNode->value->next != NULL) barf("Function names cannot contain tags.");
            if (tempNode->value->attributes != NULL) barf("Function names cannot contain tags.");
            name = tempNode->value->value;
        }
        if (tempNode->name=="_body") {
            if (tempNode->value == NULL) barf("Function _body cannot be NULL");
            bodyPtr = tempNode->value;
        }
        tempNode=tempNode->next;
    }
    tempNode = new xtpAttribute;
    tempNode->name=name;
    if (bodyPtr == NULL) barf("This function has NO BODY!");
    tempNode->value=bodyPtr;
    tempNode->next = functions->next;
    functions->next = tempNode;
}
xtpNode * findFunction(xtpAttribute * functions,string functionName)
{
    xtpAttribute * tempNode;
    tempNode=functions->next;
    xtpNode * retNode;
    retNode=NULL;
    while (tempNode!=NULL) {
        if (tempNode->name==functionName) 
        {
            retNode=tempNode->value;
        }
        tempNode=tempNode->next;
    }
    return retNode;
}
xtpNode * delNode(xtpNode * root,xtpNode * delAfter)
{
    if (delAfter != NULL) {
        if (delAfter->next == NULL) barf("Nothing to delete!");
        delAfter->next = delAfter->next->next;
    } else {
        if (root == NULL) barf ("Nothing to delete!");
        root = root->next;
    }
    return root;
}                                                               
xtpNode * insFunc(xtpNode * root,xtpNode * insertAfter,xtpNode * nodeToInsert,xtpAttribute * environ)
{
    xtpNode * tempNode;
    tempNode=nodeToInsert;
    xtpNode * tempNode2;
    tempNode2=NULL;
    xtpNode * tempNode3;
    tempNode3=tempNode2;
    if (tempNode==NULL) return root;
    while (tempNode != NULL) {
        if (tempNode2==NULL) {
            tempNode2 = new xtpNode;
            tempNode2->value=tempNode->value;
            copyAttrs(tempNode2,tempNode);
            tempNode2->next=NULL;
            tempNode3=tempNode2;
        } else {
            tempNode2->next = new xtpNode;
            tempNode2=tempNode2->next;
            tempNode2->value=tempNode->value;
            copyAttrs(tempNode2,tempNode);
            tempNode2->next=NULL;
        }
        tempNode=tempNode->next;
    }
    if (environ!=NULL) tempNode3=varReplace(tempNode3,environ);
    if (insertAfter != NULL) {
        if (insertAfter->next == NULL) barf ("insFunc used with bad insertAfter - No Next");
        tempNode2->next=insertAfter->next->next;
        insertAfter->next = tempNode3;
    } else {
        if (root == NULL) barf ("insFunc used with bad root");
        //if (root->next == NULL) barf ("insFunc used with bad insertAfter - No Next After root.");
        if (root->next != NULL) 
        {
            tempNode2->next=root->next;
        } else {
            tempNode2->next=NULL;
        }
        root=tempNode3;
    }
    return root;
}
void copyAttrs(xtpNode * to,xtpNode * from)
{
    xtpAttribute * tempNode;
    xtpAttribute * tempNode2;
    tempNode2=NULL;
    if (from != NULL) {
        tempNode=from->attributes;
        to->attributes=NULL;
        while (tempNode!=NULL)
        {
            if (tempNode2==NULL) {
                tempNode2=new xtpAttribute;
                to->attributes=tempNode2;
            } else {
                tempNode2->next = new xtpAttribute;
                tempNode2=tempNode2->next;
            }
            tempNode2->next=NULL;
            tempNode2->name=tempNode->name;
            copyNodes(tempNode2,tempNode);
            tempNode=tempNode->next;
        }
    } else {
        to->attributes=NULL;
    }
}
void copyNodes(xtpAttribute * to,xtpAttribute * from)
{
    xtpNode * tempNode;
    xtpNode * tempNode2;
    tempNode2=NULL;
    tempNode=from->value;
    if (from->value==NULL) to->value=NULL;
    while (tempNode!=NULL)
    {
        if (tempNode2==NULL) {
            tempNode2=new xtpNode;
            to->value=tempNode2;
        } else {
            tempNode2->next=new xtpNode;
            tempNode2=tempNode2->next;
        }
        tempNode2->next=NULL;
        tempNode2->value=tempNode->value;
        copyAttrs(tempNode2,tempNode);
        tempNode=tempNode->next;
    }
}
xtpNode * varReplace(xtpNode * root,xtpAttribute * environ)
{
    xtpNode * tempNode;
    xtpAttribute * tempNode2;
    xtpNode * prevNode;
    xtpNode * tempNode3;
    xtpAttribute * tempNode4;
    xtpAttribute * tempNode5;
    xtpNode * tempNode6;
    string s;
    int cont=0;
    int cont2=0;
    if (environ==NULL) barf("NULL environ in varReplace");
    
    tempNode=root;
    prevNode=NULL;
    while(tempNode != NULL)
    {
        if (tempNode->attributes != NULL)
        {
            if (tempNode->value=="param") {
                tempNode2=tempNode->attributes;
                cont=1;
                tempNode5=NULL;
                while(tempNode2 != NULL) {
                    if (tempNode2->name=="_name") {
                        tempNode5=tempNode2;
                    }
                    if (tempNode2->name=="parse") {
                        cont=0;
                    }
                    tempNode2=tempNode2->next;
                }
                if (tempNode5 != NULL) {
                    if (tempNode5->value == NULL) barf("Param _name cannot be null.");
                    if (tempNode5->value->next != NULL) barf("Name of a param cannot contain a tag.");
                    if (tempNode5->value->attributes != NULL) barf ("Name of a param cannot contain a tag.");
                    tempNode4=environ;
                    cont2=0;
                    while(tempNode4 != NULL) {
                        if (tempNode4->name==tempNode5->value->value) {
                            if (cont==0) {
                                root=insFunc(root,prevNode,tempNode4->value,NULL);
								deltree(tempNode);
                                if (prevNode==NULL) tempNode=root; else tempNode=prevNode;
                                cont2=1;
                            } else {
                                tempNode3=new xtpNode;
                                tempNode3->attributes=NULL;
                                tempNode6=tempNode4->value;
                                s="";
                                while(tempNode6!=NULL) {
                                    s+=outputNode(tempNode6);
                                    tempNode6=tempNode6->next;
                                }
                                tempNode3->value=s;
                                tempNode3->next=NULL;
                                root=insFunc(root,prevNode,tempNode3,NULL);
								deltree(tempNode);
								deltree(tempNode3);
                                //if (tempNode5->value->value=="width") barf(s);
                                if (prevNode==NULL) tempNode=root; else tempNode=prevNode;
                                cont2=1;
                            }
                            break;
                        }
                        tempNode4=tempNode4->next;
                    }
                    if (cont2==0) {
                        tempNode3 = new xtpNode;
                        tempNode3->attributes=NULL;
                        tempNode3->next= NULL;
                        tempNode3->value="";
                        root=insFunc(root,prevNode,tempNode3,NULL);
						delete tempNode3;
						deltree(tempNode);
                        if (prevNode==NULL) tempNode=root; else tempNode=prevNode;
                    }
                } else {
                    barf("Param must have a _name.");
                }
            } else if (tempNode->value != "function") {
                tempNode2=tempNode->attributes;
                while(tempNode2 != NULL) {
                    tempNode2->value=varReplace(tempNode2->value,environ);
                    tempNode2=tempNode2->next;
                }
            }
        }
        prevNode=tempNode;
        tempNode=tempNode->next;
    }
    return root;
}
string outputNode2(xtpNode * nodeToOutput,int tabify)
{
    xtpAttribute * tempNode;
    xtpNode * tempNode2;
    xtpNode * bodyVal;
    string str;
    string strTemp;
    str="";
    bodyVal=NULL;
    int i;
    tempNode=nodeToOutput->attributes;
    if (nodeToOutput->attributes == NULL)
    {
        if (nodeToOutput->value != "") {
            if (pretty) for(i=0;i<tabify;i++) str+=" ";
            str+=nodeToOutput->value;
            if (pretty) str+='\n';
        } 
    } else {
        if ((nodeToOutput->value != "function") && (nodeToOutput->value != "true") && (nodeToOutput->value != "false") && (nodeToOutput->value != "equal")) {
            if (nodeToOutput->value=="nopretty") {
                while(tempNode!=NULL) {
                    if (tempNode->name == "_body") {
                        tempNode2=tempNode->value;
                        while(tempNode2!=NULL) {
                            str+=outputNode(tempNode2);
                            tempNode2=tempNode2->next;
                        }
                    }
                    tempNode=tempNode->next;
                }
            } else {
                if (pretty) for(i=0;i<tabify;i++) str+=" ";
                str=str+"<"+nodeToOutput->value;
                while(tempNode!=NULL) {
                    if (tempNode->name == "_body") {
                        bodyVal=tempNode->value;
                    } else {
                        if (tempNode->value != NULL) {
                            str=str+" "+tempNode->name+"=\"";
                            tempNode2=tempNode->value;
                            while(tempNode2!=NULL) {
                                str=str+outputNode(tempNode2);
                                tempNode2=tempNode2->next;
                            }
                            str+="\"";
                        } else {
                            str+=" "+tempNode->name;
                        }
                    }
                    tempNode=tempNode->next;
                }
                if (bodyVal!=NULL) {
                    if ((bodyVal->attributes == NULL) && (bodyVal->next == NULL) && (bodyVal->value == "")) {
                        str=str+" />";
                    } else {
                        str=str+">";
						if (pretty) str+="\n";
                        tempNode2=bodyVal;
                        i=0;
                        while(tempNode2!=NULL) {
                            strTemp=outputNode2(tempNode2,tabify+4);
                            if (strTemp!="") {
                                str=str+strTemp;//+'\n';
                                i=1;
                            }
                            tempNode2=tempNode2->next;
                        }
                        if (pretty) if (i!=0) for(i=0;i<tabify;i++) str+=" ";
                        str=str+"</"+nodeToOutput->value+">";
                    }
                } else {
                    str=str+" />";
                }
                if (pretty) str+='\n';
            }
        }
    }
    return str;
}
string outputNode3(xtpNode * nodeToOutput,int tabify)
{
    xtpAttribute * tempNode;
    xtpNode * tempNode2;
    xtpNode * bodyVal;
    string str;
    string strTemp;
    str="";
    bodyVal=NULL;
    int i;
    tempNode=nodeToOutput->attributes;
    if (nodeToOutput->attributes == NULL)
    {
        if (nodeToOutput->value != "") {
            for(i=0;i<tabify;i++) str+=" ";
            str+=nodeToOutput->value;
            str+='\n';
        } 
    } else {
		if ((nodeToOutput->value=="dbQuery")||(nodeToOutput->value=="setVar")) {
            for(i=0;i<tabify;i++) str+=" ";
			str+=outputNode(nodeToOutput)+"\n";
        } else {
            for(i=0;i<tabify;i++) str+=" ";
            str=str+"<"+nodeToOutput->value;
            while(tempNode!=NULL) {
                if (tempNode->name == "_body") {
                    bodyVal=tempNode->value;
                } else {
                    if (tempNode->value != NULL) {
                        str=str+" "+tempNode->name+"=\"";
                        tempNode2=tempNode->value;
                        while(tempNode2!=NULL) {
                            str=str+outputNode(tempNode2);
                            tempNode2=tempNode2->next;
                        }
                        str+="\"";
                    } else {
                        str+=" "+tempNode->name;
                    }
                }
                tempNode=tempNode->next;
            }
            if (bodyVal!=NULL) {
                if ((bodyVal->attributes == NULL) && (bodyVal->next == NULL) && (bodyVal->value == "")) {
                    str=str+" />";
                } else {
                    str=str+">";
					str+="\n";
                    tempNode2=bodyVal;
                    i=0;
                    while(tempNode2!=NULL) {
                        strTemp=outputNode3(tempNode2,tabify+4);
                        if (strTemp!="") {
                            str=str+strTemp;//+'\n';
                            i=1;
                        }
                        tempNode2=tempNode2->next;
                    }
                    if (i!=0) for(i=0;i<tabify;i++) str+=" ";
                    str=str+"</"+nodeToOutput->value+">";
                }
            } else {
                str=str+" />";
            }
            str+='\n';
        }
    }
    return str;
}
unsigned char deHex(unsigned char c)
{
    if ((c>='0')&&(c<='9')) {
        return c-'0';
    } else if ((c>='a')&&(c<='f')) {
        return c-'a'+10;
    } else if ((c>='A')&&(c<='F')) {
        return c-'A'+10;
    } else return 0;
}

string eScape(string in)
{
    int i;
    string s="";
    char x[18];
    for (i=0;i<in.length();i++) {
        if ((in[i]<'0')||((in[i]>'9')&&(in[i]<'A'))||((in[i]>'Z')&&(in[i]<'a'))||(in[i]>'z')) {
            itoa((unsigned char)in[i],x,16);
            s+="%";
            s+=x;
        }
        else s += in[i];
    }
    return s;
}
string deScape(string in)
{
    int i;
    string s="";
    unsigned char v;
    int j;
    for (i=0;i<in.length();i++) {
        if (in[i]=='%') {
            j=0;
            //expect two chars
            if ((in.length()-i)>2) {
                v=(unsigned char)((deHex(in[i+1])*16)+deHex(in[i+2]));
                if (v!=0) s+=v; else {
                    s+=in[i];
                    s+=in[i+1];
                    s+=in[i+2];
                }
                i+=2;
            } else s+='%';
        } else if (in[i]=='+') 
            s+=" ";
        else s += in[i];
    }
    return s;
}
string deScape2(string in)
{
    int i;
    string s="";
    unsigned char v;
    int j;
    for (i=0;i<in.length();i++) {
        if (in[i]=='%') {
            j=0;
            //expect two chars
            if ((in.length()-i)>2) {
                v=(unsigned char)((deHex(in[i+1])*16)+deHex(in[i+2]));
                if (v!=0) {
					if (v!='\r') {
						s+=v;
					}
				} else {
                    s+=in[i];
                    s+=in[i+1];
                    s+=in[i+2];
                }
                i+=2;
            } else s+='%';
        } else if (in[i]=='+') 
            s+=" ";
        else s += in[i];
    }
    return s;
}
xtpAttribute * addAttributesFromString(xtpAttribute * base,string str)
{
    string name;
    string value;
    xtpAttribute * sbase;
    xtpAttribute * qbase;
    sbase=base;
    int fnd;
    char c;
    //name=value pairs
    //& signals another pair
    //eos is end
    int inValue=0;
    name="";
    value="";
    for(int i=0;i<str.length();i++) {
        c=str[i];
        if (c=='=') {
            if (name!="") {
                inValue=1;
            } else {
                barf("= detected before a name was given in CGI input");
            }
        } else if (c=='&') {
            if ((name!="") && (inValue==1)) {
                qbase=sbase;
                fnd=0;
                while (qbase!=NULL)
                {
                    if (qbase->name==name) {
                        qbase->value->value+="\n";
                        qbase->value->value+=deScape2(value);
                        fnd=1;
                        inValue=0;
                        value="";
                        name="";
                        break;
                    }
                    qbase=qbase->next;
                }
                if (fnd==0) {
                    base->next = new xtpAttribute;
                    base=base->next;
                    base->name=name;
                    base->value = new xtpNode;
                    base->value->attributes=NULL;
                    base->value->value=deScape2(value);
                    base->value->next=NULL;
                    base->next = NULL;
                    inValue=0;
                    value="";
                    name="";
                }
/*                base->next = new xtpAttribute;
                base=base->next;
                base->name=name;
                base->value = new xtpNode;
                base->value->attributes=NULL;
                base->value->value=deScape2(value);
                base->value->next=NULL;
                base->next = NULL;
                inValue=0;
                value="";
                name="";*/
            } else {
                barf("& detected in a very wrong place in CGI input");
            }
        } else {
			if (c!='\r') {
            if (inValue==1) {
                value+=c;
            } else {
                name+=c;
            }
			}
        }
    }
    if ((inValue) && (name!="")) {
        qbase=sbase;
        fnd=0;
        while (qbase!=NULL)
        {
            if (qbase->name==name) {
                qbase->value->value+="\n";
                qbase->value->value+=deScape2(value);
                fnd=1;
                inValue=0;
                value="";
                name="";
                break;
            }
            qbase=qbase->next;
        }
        if (fnd==0) {
            base->next = new xtpAttribute;
            base=base->next;
            base->name=name;
            base->value = new xtpNode;
            base->value->attributes=NULL;
            base->value->value=deScape2(value);
            base->value->next=NULL;
            base->next = NULL;
            inValue=0;
            value="";
            name="";
        }
    } else barf("somehow, blank or otherwise incomplete CGI input was given.");
    return base;
}
xtpAttribute * handleMultipart(string boundary,xtpAttribute * base)
{
    string name;
    string value;
    string inbuf;
    string temps,temps2;
    char x[18];
    char c;
    //xtpAttribute * sbase;
    //xtpAttribute * qbase;
    int tempint;
    //sbase=base;
    int fnum;
    int ti;
    fnum=0;
    //name has temp-filename
    //name-f has orig-filename from form
    //now we make a file in the folder "temp"
    //we just number them incrementally
    //first lets clear out the beginning boundary, --boundary
    boundary="--"+boundary;
    inbuf="";
    while(true)
    {
        cin.get(c);
        if (cin.eof()) break;
        inbuf+=c;
        if (inbuf.length()>temps.length()) 
            inbuf=inbuf.substr(1);
        if (inbuf==temps) break;
    }
    boundary="\r\n"+boundary;
    while(true)
    {
        //get headers
        //then get body
        //then boundary & repeat
        tempint=0;
        temps2="";
        while(true) 
        {
            temps="";
            //cin.getline(w,200);
            //temps=w;
            while(true)
            {
                cin.get(c);
                if (cin.eof()) break;
                if (c=='\n') break;
                if (c!='\r') temps+=c;
            }
            if (cin.eof()) break;
            if (temps=="") break; // end of headers
            //ok, look for content-disposition and possibly content-type
            //should prob. ignore content-type though
            if (temps.find("Content-Disposition")<=temps.length()) {
                //content-disposition header
                //look for filename=
                //and name=
                //set name= to the name of course
                //and should it exist, temps2 to the filename
                name=temps.substr(temps.find("name=")+5);
                if (name[0]=='"') 
                    name=name.substr(1,name.find("\"",1)-1);
                else
                    if (name.find(";")<=name.length())
                        name=name.substr(0,name.find(";"));
                if (temps.find("filename=")<=temps.length()) {
                    //it's a file
                    //set tempint
                    tempint=1;
                    //now figure out its name
                    temps2=temps.substr(temps.find("filename=")+9);
                    if (temps2[0]=='"') 
                        temps2=temps2.substr(1,temps2.find("\"",1)-1);
                    else
                        if (temps2.find(";")<=temps2.length())
                            temps2=temps2.substr(0,temps2.find(";"));
                }
            }
        }
        if (cin.eof()) break;
        //ok, we're done w/ headers.
        //we're on to the body.
        //it ends when <crlf>boundary is found.
        value="";
        if (tempint==1) {
            value="temp/"+string(itoa(fnum,x,10))+".xtf";
            xwrite.open(string("temp/"+string(itoa(fnum,x,10))+".xtf").c_str(),std::ios_base::binary);
            fnum++;
        }
        inbuf="";
        //cout<<"O_BINARY="<<O_BINARY<<" AND O_TEXT="<<O_TEXT<<'\n';
        //cout<<"setmode returned:"<<setmode( fileno( stdin ), O_BINARY )<<'\n';
        while(true)
        {
            //cin.read(&c,1);
            /*if (c=='\0' || c=='\0x1A') {
                if (tempint==1) {
                    write<<inbuf;
                    write.put(c);
                } else {
                    value+=inbuf;
                    value+=c;
                }
                inbuf="";
            } else {*/
                ti=fgetc(stdin);
                if (ti==EOF) break;
                c=ti;
                inbuf+=c;
                if (inbuf.length()>boundary.length())
                {
                    if (tempint==1)
                        xwrite.write(&inbuf[0],1);
                    else
                        value+=inbuf[0];
                    inbuf=inbuf.substr(1);
                }
                if (inbuf==boundary) break;
//            }
        }
        //setmode( fileno( stdin ), O_TEXT );
        //cout<<char(c);
        if (tempint=1) {
            xwrite.close();
        }
        //ok, build onto base
        base->next = new xtpAttribute;
        base=base->next;
        base->name=name;
        base->value = new xtpNode;
        base->value->attributes=NULL;
        base->value->value=value;
        base->value->next=NULL;
        base->next = NULL;
        if (tempint==1) {
            base->next = new xtpAttribute;
            base=base->next;
            base->name=name+"-f";
            base->value = new xtpNode;
            base->value->attributes=NULL;
            base->value->value=temps2;
            base->value->next=NULL;
            base->next = NULL;
        }
        c='-';
        cin.get(c);
        if (c=='-') break;
    }
    return base;
}
xtpAttribute * addAttributesFromStream(xtpAttribute * base,std::istream xin)
{
    string name;
    string value;
    char c;
    xtpAttribute * sbase;
    xtpAttribute * qbase;
    sbase=base;
    int fnd;
    //name=value pairs
    //& signals another pair
    //eos is end
    int inValue=0;
    name="";
    value="";
    while(true) {
        xin.get(c);
        if (xin.eof()) break;
        if (c=='=') {
            if (name!="") {
                inValue=1;
            } else {
                barf("= detected before a name was given in CGI input");
			}
        } else if (c=='&') {
            if ((name!="") && (inValue==1)) {
                qbase=sbase;
                fnd=0;
                while (qbase!=NULL)
                {
                    if (qbase->name==name) {
                        qbase->value->value+="\n";
                        qbase->value->value+=deScape2(value);
                        fnd=1;
                        inValue=0;
                        value="";
                        name="";
                        break;
                    }
                    qbase=qbase->next;
                }
                if (fnd==0) {
                    base->next = new xtpAttribute;
                    base=base->next;
                    base->name=name;
                    base->value = new xtpNode;
                    base->value->attributes=NULL;
                    base->value->value=deScape2(value);
                    base->value->next=NULL;
                    base->next = NULL;
                    inValue=0;
                    value="";
                    name="";
                }
/*                base->next = new xtpAttribute;
                base=base->next;
                base->name=name;
                base->value = new xtpNode;
                base->value->attributes=NULL;
                base->value->value=deScape2(value);
                base->value->next=NULL;
                base->next = NULL;
                inValue=0;
                value="";
                name="";*/
            } else {
                barf("& detected in a very wrong place in CGI input");
            }
        } else {
			if (c!='\r') {
            if (inValue==1) {
                value+=c;
            } else {
                name+=c;
            }
			}
		}
    }
    if ((inValue) && (name!="")) {
        qbase=sbase;
        fnd=0;
        while (qbase!=NULL)
        {
            if (qbase->name==name) {
                qbase->value->value+="\n";
                qbase->value->value+=deScape2(value);
                fnd=1;
                inValue=0;
                value="";
                name="";
                break;
            }
            qbase=qbase->next;
        }
        if (fnd==0) {
            base->next = new xtpAttribute;
            base=base->next;
            base->name=name;
            base->value = new xtpNode;
            base->value->attributes=NULL;
            base->value->value=deScape2(value);
            base->value->next=NULL;
            base->next = NULL;
            inValue=0;
            value="";
            name="";
        }
/*        base->next = new xtpAttribute;
        base=base->next;
        base->name=name;
        base->value = new xtpNode;
        base->value->attributes=NULL;
        base->value->value=deScape2(value);
        base->value->next=NULL;
        base->next = NULL;
        inValue=0;
        value="";
        name="";*/
    } else barf("somehow, blank or otherwise incomplete CGI input was given.");
    return base;
}

int main(int argc,char * argv[])
{
    xtpNode * tempNode;
    xtpNode * tempNode2;
    xtpAttribute * tempNode3;
    xtpAttribute * environ;
    int rseed;
    
    WSADATA info; 
    if (WSAStartup(MAKEWORD(1,1), &info) != 0) barf("Cannot initialize WinSock!");

    setmode( fileno( stdin ), O_BINARY );

	currDB="";
	rge=NULL;
    
	pretty=false;

    std::ifstream ifs;
    std::ofstream ofs;
    long contentlength;
    string thisfile;
    string s;
    string mtemp;
    char * cs;
    cs=getenv("HTTP_CONTENT_LENGTH");
    
    ifs.open("rseed.xtd");
    if (ifs.fail()) {
        srand(time(NULL));
        ifs.close();
        ifs.clear();
    } else {
        ifs>>rseed;
        ifs.close();
        ifs.clear();
        srand(rseed);
    }
    
    contentlength=-1;
    if (cs!=NULL) {
        contentlength=atoi(cs);
    }
    
    thisfile="";
    if (argc>1) {
        ifs.open(argv[1]);
        if (!ifs.fail()) {
            tempNode2=tagify("",ifs);
            thisfile=argv[1];
        } else {
            barf("Bad input file name.");
            //tempNode2=tagify("",cin);
        }
        ifs.close();
    } else {
        if (contentlength==-1) {
            tempNode2=tagify("",cin);
        } else {
            barf("HTTP_CONTENT_LENGTH set, but no file given");
        }
    }
        
    environ = new xtpAttribute;
    environ->next=NULL;
    environ->name="_version";
    tempNode = new xtpNode;
    tempNode->attributes=NULL;
    tempNode->next=NULL;
    tempNode->value="1.0";
    environ->value=tempNode;
    
    tempNode3=environ;
    environ->next = new xtpAttribute;
    environ=environ->next;
    
    environ->next=NULL;
    environ->name="_this";
    tempNode = new xtpNode;
    tempNode->attributes=NULL;
    tempNode->next=NULL;
    tempNode->value=thisfile;
    environ->value=tempNode;
    
    cs=getenv("QUERY_STRING");
    if (cs!=NULL) environ=addAttributesFromString(environ,cs);
    cs=getenv("HTTP_COOKIE");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_cookies";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("HTTP_REFERER");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_referer";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("AUTH_TYPE");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_auth";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("CONTENT_TYPE");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_content";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
        s=string(cs);
        if (s!="") {
            if (s.find("multipart/form-data;")<=s.length()) {
                if (s.find("boundary=")>s.length()){
                    barf("Boundary missing.");
                } else {
                    mtemp=s.substr(s.find("boundary=")+9);
                    if (mtemp[0]=='"')
                        mtemp=mtemp.substr(1,-1);
                    environ=handleMultipart(mtemp,environ);
                }
                contentlength=0;
            }
        }
    }
    cs=getenv("DOCUMENT_ROOT");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_root";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("GATEWAY_INTERFACE");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_gateway";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("PATH_INFO");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_path";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("PATH_TRANSLATED");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_path-translated";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("REMOTE_ADDR");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_remote-addr";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("REMOTE_HOST");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_remote-host";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("REMOTE_IDENT");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_remote-ident";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("REMOTE_PORT");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_remote-port";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("REMOTE_USER");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_user";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("REQUEST_URI");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_this-uri";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("REQUEST_METHOD");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_method";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("SCRIPT_NAME");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_this-name";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("SCRIPT_FILENAME");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_this-file";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("SERVER_ADMIN");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_server-admin";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("SERVER_NAME");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_server-name";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("SERVER_PORT");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_port";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("SERVER_PROTOCOL");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_protocol";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("SERVER_SIGNATURE");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_server-sig";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("SERVER_SOFTWARE");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_server-soft";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("HTTP_ACCEPT_ENCODING");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_accept-encoding";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("HTTP_ACCEPT_LANGUAGE");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_accept-lang";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("HTTP_FORWARDED");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_forwarded";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("HTTP_HOST");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_host";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("HTTP_USER_AGENT");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_user-agent";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    cs=getenv("HTTP_ACCEPT");
    if (cs!=NULL) {
        environ->next=new xtpAttribute;
        environ->next->name="_accept";
        environ->next->value=new xtpNode;
        environ->next->value->value=cs;
        environ->next->value->attributes=NULL;
        environ->next->value->next=NULL;
        environ->next->next=NULL;
        environ=environ->next;
    }
    if (contentlength>0) environ=addAttributesFromStream(environ,cin);
    
    tempNode2=varReplace(tempNode2,tempNode3);
    
    xtpAttribute * fn;
    fn=new xtpAttribute;
    fn->name="";
    fn->value=NULL;
    fn->next=NULL;
    
    vars=NULL;
    
    tempNode2=execute(tempNode2,fn,tempNode3);
    if (tempNode2->value=="barf") {
        barf(tempNode2->attributes->value->value);
    } else {
        while (tempNode2!=NULL) {
            cout<<outputNode2(tempNode2,0);
            tempNode2=tempNode2->next;
        }
    }
    /*
    for (int i=0;i<argc;i++) {
    cout<<argv[i]<<"<br />";
    }
    cout<<contentlength<<"<br />";*/
    ofs.open("rseed.xtd");
    if (!ofs.fail()) {
        ofs<<rand();
        ofs.close();
    }
    return 0;
}
