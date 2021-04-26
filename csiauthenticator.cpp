// sh start.sh -n 10 -b 20 -c 1
// tcpdump -i wlan0 dst port 5500 -X -c 6010 | tee output.txt

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <iostream>
#include <string>

#define BUFSIZE 128
#define HEADSPNUM 100
#define ACCESSPACNUM 1000
#define PACROWNUM 19
#define SCNUM 64
#define MAXTMSGSIZE 15
#define MAXCMSGSIZE 8*SCNUM
#define MAXSMSGSIZE 1

char buf[BUFSIZE];
pid_t pid1;
pid_t pid2;
pid_t pid3;
int msqid1 = 0;
int msqid2 = 0;
int msqid3 = 0;
int paccount = 0;
key_t key1;
key_t key2;
key_t key3;
bool flag = false;
bool flag2 = false;
pthread_t ntid;

struct tmsg {
	long int mtype;
	char time [MAXTMSGSIZE];
};

struct cmsg {
	long int mtype;
	char csi [MAXCMSGSIZE];
};

struct smsg {
	long int mtype;
	char signal [MAXSMSGSIZE];
}sigmsg;

void err_sys(const std::string errmsg) {
	std::cout << errmsg << std::endl;
	exit(-1);
}

void rcvpac() {
	// std::cout << "receive packets" << std::endl;
	int count = 0, ptr1 = 0, ptr2 = 0;
	struct tmsg timemsg;
	struct cmsg csimsg;
	timemsg.mtype = 1;
	csimsg.mtype = 2;

	// 读取时间戳
	do {
		if (fgets(buf, BUFSIZE, stdin) == NULL)
			err_sys("csiauthenticator: read stdin error");
	} while (buf[2] != ':');
	for (; ptr1 < 15; ++ptr1)
		timemsg.time[ptr1] = buf[ptr1];

	if (flag2) {
		// 写消息队列1
		// std::cout << "csiauthenticator: write msq1" << std::endl;
		if (msgsnd(msqid1, (void *)&timemsg, MAXTMSGSIZE,0) < 0)
			err_sys("msgsnd timemsg error");
	}

	// 读取头部两行，不保存
	if (fgets(buf, BUFSIZE, stdin) == NULL)
		err_sys("csiauthenticator: read stdin error");
	if (fgets(buf, BUFSIZE, stdin) == NULL)
		err_sys("csiauthenticator: read stdin error");

	// 读取MAC和CSI等信息
	if (fgets(buf, BUFSIZE, stdin) == NULL)
		err_sys("csiauthenticator: read stdin error");
	csimsg.csi[ptr2++] = buf[45];
	csimsg.csi[ptr2++] = buf[46];
	csimsg.csi[ptr2++] = buf[47];
	csimsg.csi[ptr2++] = buf[48];
	count++;

	while (count < 2 * SCNUM) {
		int i = 0;
		if (fgets(buf, BUFSIZE, stdin) == NULL)
			err_sys("csiauthenticator: read stdin error");
		while (count < 2 * SCNUM && i < 8) {
			csimsg.csi[ptr2++] = buf[10 + 5 * i];
			csimsg.csi[ptr2++] = buf[10 + 5 * i + 1];
			csimsg.csi[ptr2++] = buf[10 + 5 * i + 2];
			csimsg.csi[ptr2++] = buf[10 + 5 * i + 3];
			// std::cout << pacmsg.mtext[ptr - 1] << ' ' << buf[10 + 5 * i + 3] << std::endl;
			count++;
			i++;
		}
	}

	// 写消息队列2
	if (flag) {
		// std::cout << "csiauthenticator: write msq2" << std::endl;
		if (msgsnd(msqid2, (void *)&csimsg, MAXCMSGSIZE, 0) < 0)
			err_sys("csiauthenticator: msgsnd csimsg error");
	}
}

void sigusr1_handler(int signo) {
	std::cout << "csiauthenticator: received SIGUSR1" << std::endl;
	flag = true;
	// std::cout << "sigusr1_handler: true" << std::endl;
	// std::cout << "send SIGUSR1 to csiauth" << std::endl;
	// kill(pid3, SIGUSR1);
}

void *rcvp(void *arg) {
	while (true) {
		rcvpac();
		paccount++;
		if (!flag && !flag2 && paccount == HEADSPNUM) {
			flag = true;
			flag2 = true;
			std::cout << "csiauthenticator: start" << std::endl;
		}
		else if (flag && flag2 && paccount == ACCESSPACNUM + HEADSPNUM) {
			flag = false;
			flag2 = false;
			// std::cout << "rcvp: false" << std::endl;
			std::cout << "csiauthenticator: receive " << paccount << " packets" << std::endl;
		}
	}
	return ((void *)0);
}

int main(int argc, char *argv[]) {
	// 创建消息队列1
	if ((key1 = ftok("/home/pi/projects/csiauthenticator/authmsq/tmp1",1)) < 0)
		err_sys("csiauthenticator: ftok error");
	if ((msqid1 = msgget(key1, IPC_CREAT | 0666)) < 0)
		err_sys("csiauthenticator: creat msq key1 error");

	// 创建消息队列2
	if ((key2 = ftok("/home/pi/projects/csiauthenticator/authmsq/tmp2",2)) < 0)
		err_sys("csiauthenticator: ftok error");
	if ((msqid2 = msgget(key2, IPC_CREAT | 0666)) < 0)
		err_sys("csiauthenticator: creat msq key2 error");

	// 创建消息队列3
	if ((key3 = ftok("/home/pi/projects/csiauthenticator/authmsq/tmp3",3)) < 0)
		err_sys("csiauthenticator: ftok error");
	if ((msqid3 = msgget(key3, IPC_CREAT | 0666)) < 0)
		err_sys("csiauthenticator: creat msq key3 error");

	// 创建时延认证进程
	if ((pid1 = fork()) < 0)
		err_sys("csiauthenticator: fork pid1 error");
	else if (pid1 == 0) {
		if (execle("/home/pi/projects/csiauthenticator/accessauth", "accessauth") < 0)
			err_sys("csiauthenticator: process 1 exec error");
	}

	// 创建更新进程
	if ((pid2 = fork()) < 0)
		err_sys("csiauthenticator: fork pid2 error");
	else if (pid2 == 0) {
		if (execle("/home/pi/projects/csiauthenticator/updator", "updator") < 0)
			err_sys("csiauthenticator: process 2 exec error");
	}

	// 创建CSI认证进程
	if ((pid3 = fork()) < 0)
		err_sys("csiauthenticator: fork pid3 error");
	else if (pid3 == 0) {
		if (execle("/home/pi/projects/csiauthenticator/csiauth", "csiauth") < 0)
			err_sys("csiauthenticator: process 3 exec error");
	}

	// 处理SIGUSR1
	sigset_t newmask, oldmask;

        if (signal(SIGUSR1, sigusr1_handler) < 0)
                err_sys("csiauthenticator: signal(SIGUSR1) error");

        sigemptyset(&newmask);
        sigaddset(&newmask, SIGUSR1);
        if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
                err_sys("csiauthenticator: SIG_BLOCK error");

	// 创建数据包接收线程
	if (pthread_create(&ntid, NULL, rcvp, NULL) != 0)
		err_sys("csiauthenticator: pthread create error");

	// 等待SIGUSR1
	if (sigsuspend(&oldmask) != -1)
		err_sys("csiauthenticator: sigsuspend error");

	// 发送开始信号给csiauth
	std::cout << "csiauthenticator: send SIGUSR1 to csiauth" << std::endl;
	kill(pid3, SIGUSR1);
	// std::cout << "csiauthenticator: send SIGUSR1 over" << std::endl;

	while (true) {
		sleep(10);
	}

	exit(0);
}
