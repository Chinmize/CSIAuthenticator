#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <math.h>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <string>
#include "ensemble.h"

#define ACCESSPACNUM 1000
#define SCNUM 64
#define FPNUM 58
#define MAXCMSGSIZE 8*SCNUM
#define MAXSMSGSIZE 1
#define LEARNINGRATE 0.05
#define HIDDENRATIO 0.75
#define L1AESIZE 8

int msqid2 = 0;
int msqid3 = 0;
key_t key2;
key_t key3;
double fp[ACCESSPACNUM][FPNUM];
ensemble ens(FPNUM, ACCESSPACNUM, LEARNINGRATE, HIDDENRATIO, FPNUM, L1AESIZE);

struct cmsg {
	long int mtype;
	char csi [MAXCMSGSIZE];
}csimsg;

struct smsg {
	long int mtype;
	char signal [MAXSMSGSIZE];
}sigmsg;

void err_sys(const std::string errmsg) {
	std::cout << errmsg << std::endl;
	exit(-1);
}

int hstoi (std::string hs) {
    unsigned short int rst;

    std::stringstream ss;
    ss << std::hex << hs;
    ss >> rst;

    return static_cast<short int>(rst);
}

std::string readhex (int ptr) {
    std::string rst;

    rst += csimsg.csi[ptr+2];
    rst += csimsg.csi[ptr+3];
    rst += csimsg.csi[ptr];
    rst += csimsg.csi[ptr+1];

    return rst;
}

void readfp() {
	int count1 = 0, count2 = 0, count3 = 0, re = 0, im = 0;

	count1 = 0;
	while (count1 < ACCESSPACNUM) {
		count2 = 0;
		count3 = 0;
		if (msgrcv(msqid2, (void *)&csimsg, MAXCMSGSIZE, 2, 0) < 0)
			err_sys("updator: receive csimsg error");
		while (count2 < SCNUM) {
			if ((count2 > 0 && count2 < 29) || count2 > 35) {
				re = hstoi(readhex(8 * count2));
				im = hstoi(readhex(8 * count2 + 4));
				fp[count1][count3] = sqrt(re*re + im*im);
				count3++;
			}
			count2++;
		}
		count1++;
	}
}

void sndsigtom() {
	pid_t ppid = getppid();
	if (msgrcv(msqid3, (void *)&sigmsg, MAXSMSGSIZE, 3, 0) < 0)
		err_sys("updator: receive sigmsg error");
	std::cout << "updator: receive sigmsg succeeded" << std::endl;
	if (sigmsg.signal[0] == 's') {
		std::cout << "updator: authenticate succeeded" << std::endl;
		std::cout << "updator: send SIGUSR1" << std::endl;
		ens.save_model();
		kill(ppid, SIGUSR1);
	}
}

int main(int argc, char *argv[]) {
	// 打开消息队列2
	if ((key2 = ftok("/home/pi/projects/csiauthenticator/authmsq/tmp2",2)) < 0)
		err_sys("updator: ftok error");
	if ((msqid2 = msgget(key2, IPC_CREAT | 0666)) < 0)
		err_sys("updator: creat msq key2 error");

	// 打开消息队列3
	if ((key3 = ftok("/home/pi/projects/csiauthenticator/authmsq/tmp3",3)) < 0)
		err_sys("updator: ftok error");
	if ((msqid3 = msgget(key3, IPC_CREAT | 0666)) < 0)
		err_sys("updator: open msq key3 error");

	readfp();

	// 获取新模型
	for (int i = 0; i < ACCESSPACNUM; ++i) {
		ens.train(fp[i]);
	}

	sndsigtom();

	exit(0);
}
