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
//#include <fcntl.h>
//#include <unistd.h>
//#include <sys/time.h>
//#include <poll.h>

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


int main(int argc, char *argv[])
{
   //char str[256];
   //struct timeval tv;
   //struct pollfd pfd;
   //int fd;
   int ret = 0;
   //char buf[8];

   // TODO: take GPIO from arg
   ldr.gpio = 17;
   ret |= gpio_export(ldr.gpio);
   ret |= gpio_direction(ldr.gpio, GPIO_IN);
   ret |= gpio_edge(ldr.gpio, GPIO_EDGE_RISING);

   if (ret != 0) {
      gpio_unexport(ldr.gpio);
      return ret;
   }



   gpio_wait_for_interrupt(ldr.gpio, 3000);



   gpio_unexport(ldr.gpio);

   exit(0);
}
