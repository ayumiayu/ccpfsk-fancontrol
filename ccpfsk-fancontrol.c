#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <time.h>
#include <fcntl.h>

void gpio_init(void);
void gpio_h(void);
void gpio_l(void);

int debug_flag=0;

void interval_proc(void){
	int tfd;
	struct itimerspec its;
	ssize_t r_size;
	u_int64_t t_cnt;
	struct timespec curTime,lastTime;
        char buffer0[1024], buffer1[1024], buffer2[1024];
        double temp;
        char power, data;
        FILE *fp0, *fp1, *fp2;

	tfd = timerfd_create(CLOCK_MONOTONIC, 0);
	its.it_value.tv_sec = 30;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 30;
	its.it_interval.tv_nsec = 0;

	clock_gettime(CLOCK_REALTIME, &lastTime);
	timerfd_settime(tfd, 0, &its, NULL);

	memset(buffer0, 0, sizeof(buffer0));
	memset(buffer1, 0, sizeof(buffer1));
	memset(buffer2, 0, sizeof(buffer2));

	while(1){
		// get temperature
                fp0 = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
		fp1 = fopen("/sys/class/thermal/thermal_zone1/temp", "r");
		fp2 = fopen("/sys/class/thermal/thermal_zone2/temp", "r");

                fgets(buffer0, sizeof(buffer0), fp0);
                fgets(buffer1, sizeof(buffer1), fp1);
                fgets(buffer2, sizeof(buffer2), fp2);

		fclose(fp2);
		fclose(fp1);
		fclose(fp0);

                if(debug_flag == 1){
			printf("cpu-temperature0: %.1lf[℃]\n", atof(buffer0)/1000);
        	        printf("cpu-temperature1: %.1lf[℃]\n", atof(buffer1)/1000);
                	printf("cpu-temperature2: %.1lf[℃]\n", atof(buffer2)/1000);
		}

                // max temperature check
                if((buffer0 >= buffer1) && (buffer0 >= buffer2)){
                	if(debug_flag == 1){
				printf("max cpu-temperature: %.1lf[℃]\n", atof(buffer0)/1000);
			}
			temp = atof(buffer0)/1000;
		}	
                if((buffer1 >= buffer0) && (buffer1 >= buffer2)){
			if(debug_flag == 1){
				printf("max cpu-temperature: %.1lf[℃]\n", atof(buffer1)/1000);	
			}
			temp = atof(buffer1)/1000;
		}
		if((buffer2 >= buffer0) && (buffer2 >= buffer1)){
                	if(debug_flag == 1){
				printf("max cpu-temperature: %.1lf[℃]\n", atof(buffer2)/1000);
			}
			temp = atof(buffer2)/1000;
		}

		// serial communication
		if(temp >= 35.0 && temp < 50.0){     // 50%
			gpio_l();
			gpio_l();
			gpio_h();
			gpio_l();
			gpio_h();
			if(debug_flag == 1){
				printf("\n35.0~50.0 : 00101\n");
			}
		}else if(temp >= 50.0){     // 100%
			gpio_l();
                        gpio_h();
			gpio_l();
                        gpio_l();
                        gpio_h();
			if(debug_flag == 1){
				printf("\n50.0~ : 01001\n");
			}
		}else{     // 0%
			gpio_l();
                        gpio_l();
                        gpio_l();
                        gpio_h();
                        gpio_h();
			if(debug_flag == 1){
				printf("\n~35.0 : 00011\n");
			}
		}
		
		if(debug_flag == 1){
			// ここからは、インタバールを待つための処理
               		r_size = read(tfd,&t_cnt,sizeof(t_cnt));
               		// 30秒経たないとreadから帰ってこない。
			if(r_size == sizeof(t_cnt)){
				clock_gettime(CLOCK_REALTIME, &curTime);
				printf("Timer count = %lu, Now=%ld, ", t_cnt, curTime.tv_sec);
				if(curTime.tv_nsec < lastTime.tv_nsec){
					printf("Interval = %10ld.%09ld\n", curTime.tv_sec-lastTime.tv_sec-1, curTime.tv_nsec+1000000000-lastTime.tv_nsec);
				}
				else{
					printf("Interval = %10ld.%09ld\n", curTime.tv_sec-lastTime.tv_sec, curTime.tv_nsec-lastTime.tv_nsec);
				}
			}
 			lastTime = curTime;		
			sleep(1);
		}
	}
	close(tfd);

}

void gpio_init(void){
	FILE *fp0, *fp1, *fp2;

	if((fp0 = fopen("/sys/class/pwm/pwmchip0/export", "w")) == NULL){
		printf("export file open error.\n");
		exit(EXIT_FAILURE);
	}
	fputs("0", fp0);
	fclose(fp0);
	if((fp1 = fopen("/sys/class/pwm/pwmchip0/pwm0/period", "w")) == NULL){
		printf("period file open error.\n");
		exit(EXIT_FAILURE);
	}
	fputs("1000000000", fp1);
	fclose(fp1);
	if((fp2 = fopen("/sys/class/pwm/pwmchip0/pwm0/duty_cycle", "w")) == NULL){
		printf("duty_cycle file open error.\n");
		exit(EXIT_FAILURE);
	}
	fputs("983041", fp2);
	fclose(fp2);
}

void gpio_h(void){
	FILE *fp0;

	usleep(1*1000*1000);
}

void gpio_l(void){
	FILE *fp0, *fp1;

	if((fp0 = fopen("/sys/class/pwm/pwmchip0/pwm0/enable", "w")) == NULL){
		printf("enable file open error.\n");
		exit(EXIT_FAILURE);
	}
        fputs("1", fp0);
        fclose(fp0);
	if((fp1 = fopen("/sys/class/pwm/pwmchip0/pwm0/enable", "w")) == NULL){
		printf("enable file open error.\n");
		exit(EXIT_FAILURE);
	}
	fputs("0", fp1);
	fclose(fp1);
	usleep(1*1000*1000);
}

int main(int argc, char *argv[]){
	int i;

	// -dオプションの有無
	for(i=0; i<argc; i++){
		if(strcmp(argv[i], "--debug") == 0){
			debug_flag=1;
		}
	}

	gpio_init();
	interval_proc();

	return 0;

}

