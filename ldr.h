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

#include <time.h>

#define LDR_DEFAULT_HIGH_THRESHOLD                  150
#define LDR_DEFAULT_LOW_THRESHOLD                   30
#define LDR_DEFAULT_COMPLETE_DARKNESS_THRESHOLD     900
#define LDR_DEFAULT_HIGH_DURATION_MS                60000
#define LDR_DEFAULT_LOW_DURATION_MS                 300000
#define LDR_DEFAULT_COMPLETE_DARKNESS_DURATION_MS   900


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

    struct timespec cross_threshold_start_time;

    ldr_state_t state;
    unsigned int high_threshold;
    unsigned int low_threshold;
    unsigned int complete_darkness_threshold;
    unsigned int high_threshold_duration_ms;
    unsigned int low_threshold_duration_ms;
    unsigned int complete_darkness_duration_ms;

    LDRTriggerCallback trigger_cb;
    void *priv_data;
};


int ldr_init(struct ldr_sensor_t *ldr, int ldr_gpio);
void ldr_configure(struct ldr_sensor_t *ldr,
                   unsigned int high_threshold,
                   unsigned int low_threshold,
                   unsigned int complete_darkness_threshold,
                   unsigned int high_threshold_duration_ms,
                   unsigned int low_threshold_duration_ms,
                   unsigned int complete_darkness_duration_ms);
void ldr_register_callback(struct ldr_sensor_t *ldr,
                           LDRTriggerCallback cb, void *priv_data);
int ldr_read_once(struct ldr_sensor_t *ldr);
void ldr_cleanup(struct ldr_sensor_t *ldr);


#endif // _LDR_H_
