#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/resource.h>
/*** --------------------------------------------------***/

// 存取函數：取得 frequencyArray 和 secondsRecorded
int getFrequencyData(int **freqArray, size_t *seconds, size_t secondsIn, unsigned int callsIn)
{
    static int *frequencyArray = NULL;
    static size_t secondsRecorded = 0;

    if (secondsIn > 600) /* max recording time is 10 min */
        return (-1);

    if (secondsIn > 0)
    {
        if (secondsIn >= secondsRecorded)
        {
            secondsRecorded += 60; /* allocate 60 seconds each time */
            frequencyArray = realloc(frequencyArray, (secondsRecorded) * sizeof(unsigned int));
            if (!frequencyArray) {
                return (-1);
            }
        }
        frequencyArray[secondsIn] = callsIn;
    }

    if (freqArray && seconds)
    {
        *freqArray = frequencyArray;
        *seconds = secondsRecorded;
    }
    return 0;
}

// 追蹤每秒呼叫頻率的函數
void trackCallFrequency(void)
{
    static unsigned int callCount = 0; // Total count
    static unsigned int callsInCurrentSecond = 0; // Count in a second
    static time_t lastTime = 0;
    static time_t startTime = 0; // program start
    static int stopIt = 0;
/*-------------------------*/
    time_t currentTime = 0;

    if (stopIt) 
        return ;
    
    currentTime = time(NULL);
    /* first call */
    if (startTime == 0) {
        startTime = currentTime;
        lastTime = currentTime;
    }

    if (currentTime != lastTime) {
        if (callsInCurrentSecond > 0) {
            if (getFrequencyData (NULL, NULL, currentTime - startTime, callsInCurrentSecond) < 0) {
            stopIt = 1;
            return ;
            }
        }
        callsInCurrentSecond = 0; /* next seconds */
        lastTime = currentTime;
    }

    callsInCurrentSecond++;
    callCount++;
}

void printTrackFreq(void)
{
    size_t i;
    int *freqArray;
    size_t seconds;

    getFrequencyData(&freqArray, &seconds, 0, 0);

    printf("\n============================：\n");
    for (i = 0; i < seconds; i++) {
        if (freqArray[i] != 0)
            printf("sec[ %4zu ]：%9d calls\n", i , freqArray[i]);
    }
}

/*** --------------------------------------------------***/

/*
getrusage 函數可以取得程式所使用的各種系統資源統計數據，包含 CPU、記憶體、I/O 等，
所以我們也可以利用這個函數來測量程式的 CPU time：

*/
void test_getrusage(void)
{
  struct rusage ru;
  struct timeval utime;
  struct timeval stime;

  // 取得程式的 user time 與 system time
  getrusage(RUSAGE_SELF, &ru);


  utime = ru.ru_utime;
  stime = ru.ru_stime;
  double utime_used = utime.tv_sec + (double) utime.tv_usec / 1000000.0;
  double stime_used = stime.tv_sec + (double) stime.tv_usec / 1000000.0;
  printf("User Time = %f\n", utime_used);
  printf("System Time = %f\n", stime_used);
    
}
/*** --------------------------------------------------***/
#if 0 /* abandon */
static unsigned int callCount;
static clock_t start_clk;
static clock_t stop_clk;
static struct timeval start_tv;
static struct timeval stop_tv;
static struct timespec start_spc;
static struct timespec stop_spc;


int getCallCount()
{
    double 	elapsed_secsClk = 0.0;
	double 	elapsed_secsTv = 0.0;
	double 	elapsed_secsSpc = 0.0;
    int     frequency = 0.0;  // 頻率 = 呼叫次數 / 時間
    struct timespec temp;
    struct timeval diff;


	elapsed_secsClk = (double)(stop_clk - start_clk) / CLOCKS_PER_SEC;

	//if(stop_tv.tv_sec >= start_tv.tv_sec)
	//	elapsed_secsTv = (double) (stop_tv.tv_sec - start_tv.tv_sec);
    // 計算實際花費時間
    timersub(&stop_tv, &start_tv, &diff);
    elapsed_secsTv = diff.tv_sec + (double) diff.tv_usec / 1000000.0;


    if ((stop_spc.tv_nsec - start_spc.tv_nsec)<0) {
        temp.tv_sec = stop_spc.tv_sec - start_spc.tv_sec - 1;
        temp.tv_nsec = 1000000000 + stop_spc.tv_nsec - start_spc.tv_nsec;
    } else {
        temp.tv_sec = stop_spc.tv_sec - start_spc.tv_sec;
        temp.tv_nsec = stop_spc.tv_nsec - start_spc.tv_nsec;
    }
    elapsed_secsSpc = temp.tv_sec + (double) temp.tv_nsec / 1000000000.0;


printf("start_clk = %ld\n", start_clk);
printf("stop_clk  = %ld\n", stop_clk);

printf("start_tv.tv_sec = %ld\n", start_tv.tv_sec);
printf("stop_tv.tv_sec  = %ld\n", stop_tv.tv_sec);

printf("start_spc.tv_sec = %ld\n", start_spc.tv_sec);
printf("stop_spc.tv_sec  = %ld\n", stop_spc.tv_sec);


    if (elapsed_secsClk != 0)
        printf("tatal(%u)   clock-elapsed(%f) frequency(%f) \n", callCount, elapsed_secsClk, (double)callCount / elapsed_secsClk);
    if (elapsed_secsTv != 0)
        printf("tatal(%u) timeval-elapsed(%f) frequency(%f) \n", callCount, elapsed_secsTv, (double)callCount / elapsed_secsTv);
    if (elapsed_secsSpc != 0)
        printf("tatal(%u) timespec-elapsed(%f) frequency(%f) \n", callCount, elapsed_secsSpc, (double)callCount / elapsed_secsSpc);



printf("\n\n");

    return callCount;
}
void calculateFrequency(void)
{

    callCount ++;

    if (0 == start_clk)
        start_clk = clock();
    else
        stop_clk = clock();

    if (0 == start_tv.tv_sec)
        gettimeofday(&start_tv, NULL);
    else
        gettimeofday(&stop_tv, NULL);


    if (0 == start_spc.tv_sec)
        clock_gettime(CLOCK_MONOTONIC, &start_spc);
    else
        clock_gettime(CLOCK_MONOTONIC, &stop_spc);

}
#endif

/*** --------------------------------------------------***/
void testFunction()
{
    usleep(1200); /*micro-seconds*/

    trackCallFrequency();
}

void printBackground(void)
{
    struct timespec t;

    printf("CLOCKS_PER_SEC = %ld\n", CLOCKS_PER_SEC);

    clock_getres(CLOCK_MONOTONIC, &t);
    printf("Resolution: %ld nanosecond\n", t.tv_nsec);
}

int main(int argc, char *argv[])
{
    int i;

    printBackground();
    
    printf("time = %ld\n", time(NULL));
    for (i = 0; i < 10000; i++)
    {
        testFunction(); // 呼叫 testFunction 1000 次
    }
    printf("time = %ld\n", time(NULL));


    test_getrusage();
printTrackFreq();

    return 0;
}
/*
clock():
    只記錄程式本身的 CPU 使用時間。
    如果程式被系統調度器暫停、執行 I/O 操作（如 printf 或檔案讀寫）或執行 sleep 等，這些時間不會計入 clock()。

time(NULL):
記錄真實的時間流逝，包含所有等待時間（例如 I/O、系統調度、程式睡眠等）。
因此，time(NULL) 的時間差通常比 clock() 大，尤其是在程式有大量非 CPU 密集型操作時。

----------------
系統負載：如果系統負載高，程式可能被頻繁調度，導致 time(NULL) 測量的牆鐘時間比 clock() 測量的 CPU 時間長。
I/O 操作：例如，你的程式中有 printf，這類操作會消耗牆鐘時間，但不一定增加 CPU 時間。
程式結構：如果程式執行快速（例如幾毫秒），time(NULL) 可能無法捕捉到時間差，因為它的精度只有秒級。

POSIX 系統：gettimeofday() 或 clock_gettime(CLOCK_MONOTONIC)，提供微秒或奈秒級精度。



*/

