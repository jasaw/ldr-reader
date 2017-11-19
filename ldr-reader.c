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
//#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "utils.h"
#include "list.h"
#include "sysfsgpio.h"
#include "ldr.h"


#define DEFAULT_AVERAGE_BUFFER_SIZE     128
#define DEFAULT_HIGH_THRESHOLD          180
#define DEFAULT_LOW_THRESHOLD           30




struct output_gpio_t
{
    struct list_head list;
    int gpio;
    unsigned char active_low;
    int fd_gpio_value;
};


static const unsigned char usable_gpio_pins[] =
{
    2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,32,40
};
static unsigned char terminate = 0;




static int append_output_gpio(struct list_head *gpio_list_head, int gpio, unsigned char inverted)
{
    struct output_gpio_t *output_gpio = NULL;
    
    output_gpio = malloc(sizeof(struct output_gpio_t));
    if (output_gpio == NULL)
        return -1;
    memset(output_gpio, 0, sizeof(struct output_gpio_t));
    output_gpio->fd_gpio_value = -1;
    output_gpio->active_low = inverted;
    output_gpio->gpio = gpio;
    list_add_tail(&output_gpio->list, gpio_list_head);
    return 0;
}


static void remove_all_output_gpio(struct list_head *gpio_list_head)
{
    if (gpio_list_head)
    {
        struct output_gpio_t *output_gpio = NULL;
        struct list_head *entry, *__entry;
        list_for_each_safe(entry, __entry, gpio_list_head) {
            list_entry(entry, struct output_gpio_t, list, output_gpio);
            list_del(entry);
            if (output_gpio->fd_gpio_value >= 0) {
                close(output_gpio->fd_gpio_value);
                output_gpio->fd_gpio_value = -1;
                gpio_unexport(output_gpio->gpio);
            }
            free(output_gpio);
            output_gpio = NULL;
        }
    }
}


static int init_all_output_gpio(struct list_head *gpio_list_head)
{
    struct output_gpio_t *output_gpio = NULL;
    struct list_head *entry;
    int ret = 0;
    list_for_each(entry, gpio_list_head) {
        list_entry(entry, struct output_gpio_t, list, output_gpio);
        ret |= gpio_export(output_gpio->gpio);
        ret |= gpio_direction(output_gpio->gpio, GPIO_OUT);
        if (output_gpio->active_low)
            ret |= gpio_active_low(output_gpio->gpio, GPIO_ACTIVE_LOW);
        if (ret)
            return -1;
        output_gpio->fd_gpio_value = gpio_open_value(output_gpio->gpio);
        if (output_gpio->fd_gpio_value < 0)
            return -1;
    }
    return 0;
}


static int gpio_in_list(struct list_head *gpio_list_head, int gpio)
{
    struct output_gpio_t *output_gpio = NULL;
    struct list_head *entry;
    list_for_each(entry, gpio_list_head) {
        list_entry(entry, struct output_gpio_t, list, output_gpio);
        if (output_gpio->gpio == gpio)
            return 1;
    }
    return 0;
}


static void handle_terminate_signal(int sig)
{
    if ((sig == SIGTERM) || (sig == SIGINT))
        terminate = 1;
}


static void ldr_trigger_cb(void *priv_data, ldr_state_t new_state)
{
    LOG_INFO("LDR state: %d\n", new_state);

}



static void syntax(const char *progname)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [options]\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, " -g [gpiopin]    LDR GPIO pin number. Example: 17\n");
    fprintf(stderr, " -G [gpiopin]    Light change event output GPIO pin number. High when bright.\n");
    fprintf(stderr, "                 Add 'i' to invert output. Can be set multiple times.\n");
    fprintf(stderr, "                 Example: 18 or 18i.\n");
    fprintf(stderr, " -H [threshold]  High threshold in milliseconds (when dark). Default %d\n", DEFAULT_HIGH_THRESHOLD);
    fprintf(stderr, " -L [threshold]  Low threshold in milliseconds (when bright). Default %d\n", DEFAULT_LOW_THRESHOLD);
    fprintf(stderr, " -n [size]       Average over n number of samples. Default %d samples\n", DEFAULT_AVERAGE_BUFFER_SIZE);
    fprintf(stderr, " -d              Daemonize\n");
    fprintf(stderr, " -v              Increase verbose mode (can set multiple times)\n");
    fprintf(stderr, " -h              Display this help page\n");
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    const char *progname = "";
    log_level_t new_log_level = LOG_INFO;
    unsigned char daemonize = 0;
    //unsigned char found_exec = 0;
    struct ldr_sensor_t ldr;
    int ldr_gpio = -1;
    int buf_size = DEFAULT_AVERAGE_BUFFER_SIZE;
    unsigned int high_threshold = DEFAULT_HIGH_THRESHOLD;
    unsigned int low_threshold = DEFAULT_LOW_THRESHOLD;
    LIST_HEAD(output_gpio_list_head);
    int opt;
    int ret = 0;
    int i;
    char *gpio_str;


    progname = argv[0];

    while (((opt = getopt(argc, argv, "g:G:H:L:n:dvh")) != -1))
    {
        switch (opt)
        {
            case 'g':
                ldr_gpio = atoi(optarg);
                for (i = 0; i < sizeof(usable_gpio_pins)/sizeof(usable_gpio_pins[0]); i++) {
                    if (usable_gpio_pins[i] == ldr_gpio)
                        break;
                }
                if (i >= sizeof(usable_gpio_pins)/sizeof(usable_gpio_pins[0])) {
                    LOG_ERROR("Error: Invalid LDR GPIO pin %d\n", ldr_gpio);
                    exit(EXIT_FAILURE);
                }
                LOG_VERBOSE("LDR GPIO pin %d\n", ldr_gpio);
                break;

            case 'G':
                {
                    // look for "i" suffix
                    unsigned char inverted = 0;
                    char *gpio_str = optarg;
                    int gpio_str_len = strlen(gpio_str);
                    if (gpio_str_len > 0) {
                        if (gpio_str[gpio_str_len-1] == 'i') {
                            inverted = 1;
                            gpio_str[gpio_str_len-1] = 0;
                        }
                    }
                    int output_gpio = atoi(gpio_str);
                    for (i = 0; i < sizeof(usable_gpio_pins)/sizeof(usable_gpio_pins[0]); i++) {
                        if (usable_gpio_pins[i] == output_gpio)
                            break;
                    }
                    if (i >= sizeof(usable_gpio_pins)/sizeof(usable_gpio_pins[0])) {
                        LOG_ERROR("Error: Invalid output GPIO pin %d\n", output_gpio);
                        exit(EXIT_FAILURE);
                    }
                    if (gpio_in_list(&output_gpio_list_head, output_gpio)) {
                        LOG_ERROR("Error: output GPIO pin %d already specified\n", output_gpio);
                        exit(EXIT_FAILURE);
                    }
                    if (inverted)
                        LOG_VERBOSE("Output GPIO pin %d is inverted\n", output_gpio);
                    LOG_VERBOSE("Adding output GPIO pin %d to list\n", output_gpio);
                    if (append_output_gpio(&output_gpio_list_head, output_gpio, inverted)) {
                        LOG_ERROR("Error: Unable to add output GPIO pin %d to list\n", output_gpio);
                        exit(EXIT_FAILURE);
                    }
                }
                break;

            case 'H':
                high_threshold = atoi(optarg);
                if (high_threshold < 0) {
                    LOG_ERROR("Error: Invalid high threshold %d\n", high_threshold);
                    exit(EXIT_FAILURE);
                }
                break;

            case 'L':
                low_threshold = atoi(optarg);
                if (low_threshold < 0) {
                    LOG_ERROR("Error: Invalid low threshold %d\n", low_threshold);
                    exit(EXIT_FAILURE);
                }
                break;

            case 'n':
            {
                int new_buf_size = atoi(optarg);
                if (new_buf_size <= 0) {
                    LOG_ERROR("Error: Invalid buffer size %d\n", new_buf_size);
                    exit(EXIT_FAILURE);
                }
                buf_size = new_buf_size;
            }
            break;

            //case 'x': found_exec = 1; break;
            case 'd': daemonize = 1; break;
            case 'v': new_log_level++; break;
            case 'h': // fall through
            default:
                syntax(progname);
                break;
        }
    }

    // too many arguments given
    if (optind < argc) {
        LOG_ERROR("Error: Too many arguments given\n");
        exit(EXIT_FAILURE);
    }



    set_log_level(new_log_level);

    if (ldr_gpio < 0) {
        LOG_ERROR("Error: LDR GPIO pin not specified\n");
        exit(EXIT_FAILURE);
    }
    if (gpio_in_list(&output_gpio_list_head, ldr_gpio)) {
        LOG_ERROR("Error: LDR GPIO pin %d is used as output\n", ldr_gpio);
        exit(EXIT_FAILURE);
    }

    if (high_threshold <= low_threshold) {
        LOG_ERROR("Error: high threshold must be greater than low threshold\n");
        exit(EXIT_FAILURE);
    }


    signal(SIGINT, handle_terminate_signal);
    signal(SIGTERM, handle_terminate_signal);

    if (ldr_init(&ldr, ldr_gpio, buf_size, high_threshold, low_threshold)) {
        LOG_ERROR("Error: Failed to initialize LDR GPIO pin\n");
        ret = -1;
        goto clean_up;
    }

    ldr_register_callback(&ldr, ldr_trigger_cb, NULL);

    // init output GPIO pins
    if (init_all_output_gpio(&output_gpio_list_head)) {
        LOG_ERROR("Error: Failed to initialize output GPIO pins\n");
        ret = -1;
        goto clean_up;
    }


    while (!terminate) {
        ldr_read_once(&ldr);
    }



clean_up:
    remove_all_output_gpio(&output_gpio_list_head);
    ldr_cleanup(&ldr);

    exit(ret);
}
