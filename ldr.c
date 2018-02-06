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
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "sysfsgpio.h"
#include "ldr.h"


void ldr_cleanup(struct ldr_sensor_t *ldr)
{
    if (ldr->buf) {
        free(ldr->buf);
        ldr->buf = NULL;
    }
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


int ldr_init(struct ldr_sensor_t *ldr, int ldr_gpio, unsigned int buf_size,
             unsigned int high_threshold, unsigned int low_threshold)
{
    memset(ldr, 0, sizeof(struct ldr_sensor_t));
    ldr->gpio = -1;
    ldr->fd_gpio_direction = -1;
    ldr->fd_gpio_edge = -1;
    ldr->fd_gpio_value = -1;

    if (buf_size == 0)
        return -1;
    ldr->buf = malloc(sizeof(unsigned int) * buf_size);
    if (ldr->buf == NULL)
        return -1;
    memset(ldr->buf, 0, sizeof(unsigned int) * buf_size);
    ldr->buf_size = buf_size;

    ldr->high_threshold = high_threshold;
    ldr->low_threshold = low_threshold;

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


void ldr_register_callback(struct ldr_sensor_t *ldr,
                           LDRTriggerCallback cb, void *priv_data)
{
    ldr->trigger_cb = cb;
    ldr->priv_data = priv_data;

}


static void ldr_update_running_average(struct ldr_sensor_t *ldr,
                                       int time_diff_ms)
{
    ldr->running_total -= ldr->buf[ldr->buf_index];
    ldr->buf[ldr->buf_index] = time_diff_ms;
    ldr->running_total += ldr->buf[ldr->buf_index];
    ldr->buf_index++;
    if (ldr->buf_index >= ldr->buf_size)
        ldr->buf_index = 0;
    ldr->running_avg = ldr->running_total / ldr->buf_size;
    
    if (ldr->state == LDR_BRIGHT) {
        if (ldr->running_avg > ldr->high_threshold) {
            ldr->state = LDR_DARK;
            if (ldr->trigger_cb)
                ldr->trigger_cb(ldr->priv_data, ldr->state);
        }
    } else if (ldr->state == LDR_DARK) {
        if (ldr->running_avg < ldr->low_threshold) {
            ldr->state = LDR_BRIGHT;
            if (ldr->trigger_cb)
                ldr->trigger_cb(ldr->priv_data, ldr->state);
        }
    } else {
        if (time_diff_ms < (ldr->high_threshold + ldr->low_threshold)/2)
            ldr->state = LDR_BRIGHT;
        else
            ldr->state = LDR_DARK;
        if (ldr->trigger_cb)
            ldr->trigger_cb(ldr->priv_data, ldr->state);
    }
}


int ldr_read_once(struct ldr_sensor_t *ldr)
{
    struct timespec start_time;
    struct timespec now;
    int time_diff_ms;
    int poll_ret;
    int ret = 0;

    // drain capacitor
    ret |= gpio_write_string(ldr->fd_gpio_direction, "out\n", "direction");
    ret |= gpio_write_string(ldr->fd_gpio_value, "0\n", "value");
    udelay(100000);
    // change to input to let capacitor charge
    ret |= gpio_write_string(ldr->fd_gpio_direction, "in\n", "direction");
    ret |= gpio_write_string(ldr->fd_gpio_edge, "rising\n", "edge");
    if (ret != 0)
        return ret;
    // time the interrupt
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    poll_ret = gpio_wait_for_interrupt_fd(ldr->fd_gpio_value, 1000);
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (poll_ret >= 0) {
        time_diff_ms = (now.tv_sec - start_time.tv_sec) * 1000 + (now.tv_nsec - start_time.tv_nsec) / 1000000;
        ldr_update_running_average(ldr, time_diff_ms);
        LOG_VERBOSE("%d ms, avg %d ms\n", time_diff_ms, ldr->running_avg);
    }
    // reset the edge back to none
    ret |= gpio_write_string(ldr->fd_gpio_edge, "none\n", "edge");

    return ret;
}
