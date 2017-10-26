/*
 *    Filename: sysfsgpio.h
 * Description: sysfsgpio.
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

#ifndef _SYSFSGPIO_H_
#define _SYSFSGPIO_H_


#define GPIO_IN  0
#define GPIO_OUT 1

#define GPIO_LOW  0
#define GPIO_HIGH 1

#define GPIO_EDGE_NONE      0
#define GPIO_EDGE_RISING    1
#define GPIO_EDGE_FALLING   2
#define GPIO_EDGE_BOTH      3

#define GPIO_ACTIVE_HIGH    0
#define GPIO_ACTIVE_LOW     1


int gpio_export(int pin);
int gpio_unexport(int pin);
int gpio_open_direction(int pin);
int gpio_direction(int pin, int dir);
int gpio_open_edge(int pin);
int gpio_edge(int pin, int edge);
int gpio_open_active_low(int pin);
int gpio_active_low(int pin, int active_high_low);
int gpio_open_value(int pin);
int gpio_read(int pin);
int gpio_write(int pin, int value);
int gpio_wait_for_interrupt_fd(int fd, int timeout_ms);
int gpio_wait_for_interrupt(int pin, int timeout_ms);
int gpio_write_string(int fd, const char *str, const char *filename);


#endif // _SYSFSGPIO_H_
