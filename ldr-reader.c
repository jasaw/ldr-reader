/*
 *    Filename: rpi-ldr-reader.c
 * Description: Raspberry Pi Light Dependent Resistor reader.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "utils.h"
#include "sysfsgpio.h"


struct ldr_t
{
   int gpio;
};

static struct ldr_t ldr =
{
   .gpio = -1,
};
static unsigned char terminate = 0;



void handle_terminate_signal(int sig)
{
   if ((sig == SIGTERM) || (sig == SIGINT))
      terminate = 1;
}


int main(int argc, char *argv[])
{
   //char str[256];
   //struct timeval tv;
   //struct pollfd pfd;
   //int fd;
   struct timespec start_time;
   struct timespec now;
   int ret = 0;
   //char buf[8];

   set_log_level(LOG_DEBUG);

   // TODO: take GPIO from arg
   ldr.gpio = 17;

   signal(SIGINT, handle_terminate_signal);
   signal(SIGTERM, handle_terminate_signal);

   ret |= gpio_export(ldr.gpio);



   int time_diff_ms;
   int poll_ret;
   while (!terminate) {
      // drain capacitor
      //udelay(100000);
      ret |= gpio_direction(ldr.gpio, GPIO_OUT);
      //udelay(100000);
      ret |= gpio_write(ldr.gpio, GPIO_LOW);
      udelay(100000);
      // change to input to let capacitor charge
      ret |= gpio_direction(ldr.gpio, GPIO_IN);
      ret |= gpio_edge(ldr.gpio, GPIO_EDGE_RISING);
      if (ret != 0)
         break;

      clock_gettime(CLOCK_MONOTONIC, &start_time);
      poll_ret = gpio_wait_for_interrupt(ldr.gpio, 3000);
      clock_gettime(CLOCK_MONOTONIC, &now);
      if (poll_ret >= 0) {
         time_diff_ms = (now.tv_sec - start_time.tv_sec) * 1000 + (now.tv_nsec - start_time.tv_nsec) / 1000000;
         LOG_VERBOSE("%d ms\n", time_diff_ms);
      }
      ret |= gpio_edge(ldr.gpio, GPIO_EDGE_NONE);
   }



clean_up:
   gpio_unexport(ldr.gpio);

   exit(0);
}
