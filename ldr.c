/*
 *    Filename: ldr.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "sysfsgpio.h"
#include "ldr.h"


void ldr_cleanup(struct ldr_sensor_t *ldr)
{
    if (ldr->fd_gpio_direction >= 0) {
        close(ldr->fd_gpio_direction);
        ldr->fd_gpio_direction = -1;
    }
    if (ldr->fd_gpio_edge >= 0) {
        close(ldr->fd_gpio_edge);
        ldr->fd_gpio_edge = -1;
    }
    if (ldr->fd_gpio_value >= 0) {
        close(ldr->fd_gpio_value);
        ldr->fd_gpio_value = -1;
    }
    if (ldr->gpio != -1) {
        gpio_unexport(ldr->gpio);
        ldr->gpio = -1;
    }
}


int ldr_init(struct ldr_sensor_t *ldr, int ldr_gpio)
{
    memset(ldr, 0, sizeof(struct ldr_sensor_t));
    ldr->gpio = -1;
    ldr->fd_gpio_direction = -1;
    ldr->fd_gpio_edge = -1;
    ldr->fd_gpio_value = -1;
    ldr->fd_raw_value_log_file = -1;

    clock_gettime(CLOCK_MONOTONIC, &(ldr->cross_threshold_start_time));
    ldr->high_threshold = LDR_DEFAULT_HIGH_THRESHOLD;
    ldr->low_threshold = LDR_DEFAULT_LOW_THRESHOLD;
    ldr->complete_darkness_threshold = LDR_DEFAULT_COMPLETE_DARKNESS_THRESHOLD;
    ldr->high_threshold_duration_ms = LDR_DEFAULT_HIGH_DURATION_MS;
    ldr->low_threshold_duration_ms = LDR_DEFAULT_LOW_DURATION_MS;
    ldr->complete_darkness_duration_ms = LDR_DEFAULT_COMPLETE_DARKNESS_DURATION_MS;

    if (gpio_export(ldr_gpio)) {
        return -1;
    }
    ldr->gpio = ldr_gpio;
    ldr->fd_gpio_direction = gpio_open_direction(ldr->gpio);
    ldr->fd_gpio_edge = gpio_open_edge(ldr->gpio);
    ldr->fd_gpio_value = gpio_open_value(ldr->gpio);
    if ((ldr->fd_gpio_direction < 0) ||
        (ldr->fd_gpio_edge < 0) ||
        (ldr->fd_gpio_value < 0)) {
        ldr_cleanup(ldr);
        return -1;
    }
    return 0;
}


void ldr_configure(struct ldr_sensor_t *ldr,
                   unsigned int high_threshold,
                   unsigned int low_threshold,
                   unsigned int complete_darkness_threshold,
                   unsigned int high_threshold_duration_ms,
                   unsigned int low_threshold_duration_ms,
                   unsigned int complete_darkness_duration_ms)
{
    ldr->high_threshold = high_threshold;
    ldr->low_threshold = low_threshold;
    ldr->complete_darkness_threshold = complete_darkness_threshold;
    ldr->high_threshold_duration_ms = high_threshold_duration_ms;
    ldr->low_threshold_duration_ms = low_threshold_duration_ms;
    ldr->complete_darkness_duration_ms = complete_darkness_duration_ms;
}


void ldr_register_callback(struct ldr_sensor_t *ldr,
                           LDRTriggerCallback cb, void *priv_data)
{
    ldr->trigger_cb = cb;
    ldr->priv_data = priv_data;

}


static void ldr_update_state(struct ldr_sensor_t *ldr, int ldr_duration_ms,
                             struct timespec *now)
{
    int time_diff_ms = (now->tv_sec - ldr->cross_threshold_start_time.tv_sec) * 1000 + (now->tv_nsec - ldr->cross_threshold_start_time.tv_nsec) / 1000000;
    if (ldr->state == LDR_BRIGHT) {
        if (ldr_duration_ms >= ldr->high_threshold) {
            if (((ldr_duration_ms >= ldr->complete_darkness_threshold) &&
                 (time_diff_ms >= ldr->complete_darkness_duration_ms)) ||
                (time_diff_ms >= ldr->high_threshold_duration_ms)) {
                ldr->state = LDR_DARK;
                memcpy(&(ldr->cross_threshold_start_time), now, sizeof(struct timespec));
                if (ldr->trigger_cb)
                    ldr->trigger_cb(ldr->priv_data, ldr->state);
            }
        } else
            memcpy(&(ldr->cross_threshold_start_time), now, sizeof(struct timespec));
    } else if (ldr->state == LDR_DARK) {
        if (ldr_duration_ms < ldr->low_threshold) {
            if (time_diff_ms >= ldr->low_threshold_duration_ms) {
                ldr->state = LDR_BRIGHT;
                memcpy(&(ldr->cross_threshold_start_time), now, sizeof(struct timespec));
                if (ldr->trigger_cb)
                    ldr->trigger_cb(ldr->priv_data, ldr->state);
            }
        } else
            memcpy(&(ldr->cross_threshold_start_time), now, sizeof(struct timespec));
    } else {
        if ((ldr_duration_ms >= ldr->complete_darkness_threshold) ||
            (ldr_duration_ms >= (ldr->high_threshold + ldr->low_threshold)/2))
            ldr->state = LDR_DARK;
        else
            ldr->state = LDR_BRIGHT;
        memcpy(&(ldr->cross_threshold_start_time), now, sizeof(struct timespec));
        if (ldr->trigger_cb)
            ldr->trigger_cb(ldr->priv_data, ldr->state);
    }
    if (ldr->fd_raw_value_log_file >= 0) {
        char rawbuf[3];
        rawbuf[2] = (char)(ldr->state);
        rawbuf[1] = (char)((ldr_duration_ms >> 8) & 0xFF);
        rawbuf[0] = (char)(ldr_duration_ms & 0xFF);
        write(ldr->fd_raw_value_log_file, rawbuf, sizeof(rawbuf));
    }
}


int ldr_read_once(struct ldr_sensor_t *ldr)
{
    struct timespec start_time;
    struct timespec now;
    int time_diff_ms = 0;
    int poll_ret;
    int ret = 0;

    // drain capacitor
    ret |= gpio_write_string(ldr->fd_gpio_direction, "out\n", "direction");
    ret |= gpio_write_string(ldr->fd_gpio_value, "0\n", "value");
    udelay(100000);
    // no need to read too quickly, add additional delay
    if (time_diff_ms < 250)
        udelay((250 - time_diff_ms)*1000);
    // change to input to let capacitor charge
    ret |= gpio_write_string(ldr->fd_gpio_direction, "in\n", "direction");
    ret |= gpio_write_string(ldr->fd_gpio_edge, "rising\n", "edge");
    if (ret != 0)
        return ret;
    // time the interrupt
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    poll_ret = gpio_wait_for_interrupt_fd(ldr->fd_gpio_value, 400);
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (poll_ret >= 0) {
        time_diff_ms = (now.tv_sec - start_time.tv_sec) * 1000 + (now.tv_nsec - start_time.tv_nsec) / 1000000;
        ldr_update_state(ldr, time_diff_ms, &now);
        LOG_VERBOSE("%d ms\n", time_diff_ms);
    }
    // reset the edge back to none
    ret |= gpio_write_string(ldr->fd_gpio_edge, "none\n", "edge");

    return ret;
}
