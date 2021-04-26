#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <iostream>
#include "ensemble.h"

#define SCNUM 64
#define FPNUM 56
#define MAXCMSGSIZE 8*SCNUM
#define LEARNINGRATE 0.05
#define HIDDENRATIO 0.75
#define L1AESIZE 8

int msqid2 = 0;
key_t key2;
double fp[FPNUM];
ensemble ens(FPNUM, 0, LEARNINGRATE, HIDDENRATIO, FPNUM, L1AESIZE);

struct cmsg {
	long int mtype;
	char csi[MAXCMSGSIZE];
}csimsg;

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
	int count1 = 0, count2 = 0, re = 0, im = 0;
	if (msgrcv(msqid2, (void *)&csimsg, MAXCMSGSIZE, 2, 0) < 0)
		err_sys("csiauth: receive csimsg error");
	// std::cout << "csiauth: readfp" << std::endl;
	while (count1 < SCNUM) {
		if ((count1 > 0 && count1 < 29) || count1 > 35) {
			re = hstoi(readhex(8 * count1));
			im = hstoi(readhex(8 * count1 + 4));
			fp[count2] = sqrt(re*re + im*im);
			// printf("%lf, ", fp[count2]);
			count2++;
		}
		count1++;

	}
	// printf("\n");
}

void csiauth() {
	while (true) {
		// std::cout << "csiauth: do authentication ..." << std::endl;
		readfp();
		double rmse = ens.process(fp);
		printf("%lf\n", rmse);
	}
}

void sigusr1_handler(int signo) {
	std::cout << "csiauth: received SIGUSR1" << std::endl;
}

int main(int argc, char *argv[]) {
	sigset_t newmask, oldmask;

	if (signal(SIGUSR1, sigusr1_handler) < 0)
		err_sys("csiauth: signal(SIGUSR1) error");

	sigemptyset(&newmask);
	sigaddset(&newmask, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
		err_sys("csiauth: SIG_BLOCK error");
	// else
	// 	std::cout << "csiauth: block SIGUSR1" << std::endl;

	// 打开消息队列2
	if ((key2 = ftok("/home/pi/projects/csiauthenticator/authmsq/tmp2",2)) < 0)
		err_sys("csiauth: ftok error");
	if ((msqid2 = msgget(key2, IPC_CREAT | 0666)) < 0)
		err_sys("csiauth: creat msq key2 error");

	// std::cout << "csiauth: wait for SIGUSR1" << std::endl;
	// 阻塞等待信号SIGUSR1
	if (sigsuspend(&oldmask) != -1)
		err_sys("csiauth: sigsuspend error");

	// 读取认证模型
	std::cout << "csiauth: load model" << std::endl;
	ens.load_model();
	std::cout << "csiauth: model load over" << std::endl;

	// 接收CSI，并输出认证结果
	csiauth();

	exit(0);
}
