#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <math.h>
#include <string>
#include <iostream>

#include "ensemble.h"

#define ACCESSPACNUM 1000
#define MAXTMSGSIZE 15
#define MAXSMSGSIZE 1
#define FPLEN 20
#define TI_MIN 9
#define TI_MAX 11
#define LEARNINGRATE 0.05
#define HIDDENRATIO 0.75
#define L1AESIZE 5

double time_interval[ACCESSPACNUM];
double fp[FPLEN];
int msqid1 = 0;
int msqid3 = 0;
key_t key1;
key_t key3;
bool auth_rst = false;

struct tmsg {
	long int mtype;
	char time[MAXTMSGSIZE];
}timemsg;

struct smsg {
	long int mtype;
	char signal[MAXSMSGSIZE];
}sigmsg;

void err_sys(const std::string errmsg) {
	std::cout << errmsg << std::endl;
	exit(-1);
}

double gettime() {
	int ptr = 14, base = 1;
    double rst = 0;

    for (int i = 0; i < 6; ++i) {
        rst += base * (timemsg.time[ptr--] - '0');
        base *= 10;
    }
    rst /= 1000000;
    ptr--;
    rst += (timemsg.time[ptr--] - '0');
    rst += 10 * (timemsg.time[ptr--] - '0');
    ptr--;
    rst += 60 * (timemsg.time[ptr--] - '0');
    rst += 600 * (timemsg.time[ptr--] - '0');
    ptr--;
    rst += 3600 * (timemsg.time[ptr--] - '0');
    rst += 36000 * (timemsg.time[ptr--] - '0');

    return rst;
}

void readti() {
	int count = 0;

	double time[2];
	time[0] = 0;
	while (count < ACCESSPACNUM) {
		if (msgrcv(msqid1, (void *)&timemsg, MAXTMSGSIZE, 1, 0) < 0)
			err_sys("accessauth: receive timemsg error");
		time[1] = gettime();
		// std::cout << time[1] - time[0] << std::endl;
		time_interval[count++] = time[1] - time[0];
		time[0] = time[1];
	}
	time_interval[0] = time_interval[1];
}

void generator() {
	for (int i = 0; i < ACCESSPACNUM; ++i) {
		if (time_interval[i] >= TI_MIN && time_interval[i] <= TI_MAX) {
			int ptr = ceil((time_interval[i] - TI_MIN) / ((TI_MAX - TI_MIN) / (double)FPLEN));
			fp[ptr]++;
		}
	}
}

bool authenticate() {
	ensemble ens(FPLEN, 0, LEARNINGRATE, HIDDENRATIO, FPLEN, L1AESIZE);
	ens.load_model();

	double rmse = ens.execute(fp);
	return rmse < 0.2;
}

void sndsig() {
	sigmsg.mtype = 3;
	sigmsg.signal[0] = (auth_rst ? 's' : 'f');
	if (msgsnd(msqid3, (void *)&sigmsg, MAXSMSGSIZE, 0) < 0)
		err_sys("accessauth: msgsnd sigmsg error");
	std::cout << "accessauth: sndsig(" << sigmsg.signal[0] << ")" << std::endl;
}

int main(int argc, char *argv[]) {

	// 打开消息队列1
	if ((key1 = ftok("/home/pi/projects/csiauthenticator/authmsq/tmp1",1)) < 0)
		err_sys("accessauth: ftok error");
	if ((msqid1 = msgget(key1, IPC_CREAT | 0666)) < 0)
		err_sys("accessauth: open msq key1 error");

	// 打开消息队列3
	if ((key3 = ftok("/home/pi/projects/csiauthenticator/authmsq/tmp3",3)) < 0)
		err_sys("accessauth: ftok error");
	if ((msqid3 = msgget(key3, IPC_CREAT | 0666)) < 0)
		err_sys("accessauth: open msq key3 error");

	// 读取时间间隔
	readti();

	// 提取时间间隔密度指纹
	generator();

	// 认证
	// auth_rst = authenticate();
	auth_rst = true;

	// 发送认证结束信号
	sndsig();

	exit(0);
}
