/*
 *    Filename: ldr.h
 * Description: ldr.
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

#ifndef _LDR_H_
#define _LDR_H_


typedef enum
{
    LDR_UNKNOWN = 0,
    LDR_BRIGHT,
    LDR_DARK
} ldr_state_t;

typedef void (*LDRTriggerCallback)(void *priv_data, ldr_state_t new_state);

struct ldr_sensor_t
{
    int gpio;

    int fd_gpio_direction;
    int fd_gpio_edge;
    int fd_gpio_value;

    int buf_index;
    unsigned int buf_size;
    unsigned int *buf;
    unsigned int running_total;
    unsigned int running_avg;

    ldr_state_t state;
    unsigned int high_threshold;
    unsigned int low_threshold;

    LDRTriggerCallback trigger_cb;
    void *priv_data;
};


int ldr_init(struct ldr_sensor_t *ldr, int ldr_gpio, unsigned int buf_size,
             unsigned int high_threshold, unsigned int low_threshold);
void ldr_register_callback(struct ldr_sensor_t *ldr,
                           LDRTriggerCallback cb, void *priv_data);
int ldr_read_once(struct ldr_sensor_t *ldr);
void ldr_cleanup(struct ldr_sensor_t *ldr);


#endif // _LDR_H_
