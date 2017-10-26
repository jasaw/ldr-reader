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
#include <unistd.h>

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

   int fd_gpio_direction = -1;
   int fd_gpio_edge = -1;
   int fd_gpio_value = -1;
   int time_diff_ms;
   int poll_ret;
   char *gpio_str;

   ret |= gpio_export(ldr.gpio);
   fd_gpio_direction = gpio_open_direction(ldr.gpio);
   fd_gpio_edge = gpio_open_edge(ldr.gpio);
   fd_gpio_value = gpio_open_value(ldr.gpio);
   if ((ret != 0) ||
       (fd_gpio_direction < 0) ||
       (fd_gpio_edge < 0) ||
       (fd_gpio_value < 0))
      goto clean_up;


   while (!terminate) {
      // drain capacitor
      gpio_str = "out\n";
      ret |= gpio_write_string(fd_gpio_direction, gpio_str, "direction");
      gpio_str = "0\n";
      ret |= gpio_write_string(fd_gpio_value, gpio_str, "value");
      udelay(100000);
      // change to input to let capacitor charge
      gpio_str = "in\n";
      ret |= gpio_write_string(fd_gpio_direction, gpio_str, "direction");
      gpio_str = "rising\n";
      ret |= gpio_write_string(fd_gpio_edge, gpio_str, "edge");
      if (ret != 0)
         break;
      // time the interrupt
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      poll_ret = gpio_wait_for_interrupt_fd(fd_gpio_value, 3000);
      clock_gettime(CLOCK_MONOTONIC, &now);
      if (poll_ret >= 0) {
         time_diff_ms = (now.tv_sec - start_time.tv_sec) * 1000 + (now.tv_nsec - start_time.tv_nsec) / 1000000;
         LOG_VERBOSE("%d ms\n", time_diff_ms);
      }
      // reset the edge back to none
      gpio_str = "none\n";
      ret |= gpio_write_string(fd_gpio_edge, gpio_str, "edge");
   }



clean_up:
   if (fd_gpio_direction >= 0)
      close(fd_gpio_direction);
   if (fd_gpio_edge >= 0)
      close(fd_gpio_edge);
   if (fd_gpio_value >= 0)
      close(fd_gpio_value);
   gpio_unexport(ldr.gpio);

   exit(0);
}
