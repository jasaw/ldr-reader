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
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <wordexp.h>

#include "utils.h"
#include "list.h"
#include "sysfsgpio.h"
#include "ldr.h"



struct output_gpio_t
{
    struct list_head list;
    int gpio;
    unsigned char active_low;
    int fd_gpio_value;
};


struct trigger_action_t
{
    struct list_head gpio_list_head;
    const char *cmd_bright;
    const char *cmd_dark;
    wordexp_t cmd_bright_exp_result;
    wordexp_t cmd_dark_exp_result;
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


static void trigger_action_cleanup(struct trigger_action_t *action)
{
    remove_all_output_gpio(&(action->gpio_list_head));
    if (action->cmd_bright) {
        wordfree(&(action->cmd_bright_exp_result));
        action->cmd_bright = NULL;
    }
    if (action->cmd_dark) {
        wordfree(&(action->cmd_dark_exp_result));
        action->cmd_dark = NULL;
    }
}


static void trigger_action_init(struct trigger_action_t *action)
{
    memset(action, 0, sizeof(struct trigger_action_t));
    INIT_LIST_HEAD(&(action->gpio_list_head));
}


static void handle_terminate_signal(int sig)
{
    if ((sig == SIGTERM) || (sig == SIGINT))
        terminate = 1;
}


static void ldr_trigger_cb(void *priv_data, ldr_state_t new_state)
{
    struct trigger_action_t *action = (struct trigger_action_t *)priv_data;
    struct output_gpio_t *output_gpio = NULL;
    struct list_head *entry;
    int ret = 0;
    const char *gpio_str;

    LOG_INFO("LDR state: %d\n", new_state);

    if (new_state == LDR_DARK)
        gpio_str = "0\n";
    else
        gpio_str = "1\n";

    list_for_each(entry, &(action->gpio_list_head)) {
        list_entry(entry, struct output_gpio_t, list, output_gpio);
        ret |= gpio_write_string(output_gpio->fd_gpio_value, gpio_str, "value");
    }

    if (new_state == LDR_DARK) {
        if (action->cmd_dark) {
            pid_t pid = fork();
            if (pid == 0) {
                // This is the child process.  Execute the command.
                execv(action->cmd_dark_exp_result.we_wordv[0], action->cmd_dark_exp_result.we_wordv);
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                // The fork failed
                LOG_ERROR("Error: fork failed: %s\n", action->cmd_dark);
            } else {
                // This is the parent process, do nothing.  */
            }
        }
    } else {
        if (action->cmd_bright) {
            pid_t pid = fork();
            if (pid == 0) {
                // This is the child process.  Execute the command.
                execv(action->cmd_bright_exp_result.we_wordv[0], action->cmd_bright_exp_result.we_wordv);
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                // The fork failed
                LOG_ERROR("Error: fork failed: %s\n", action->cmd_bright);
            } else {
                // This is the parent process, do nothing.  */
            }
        }
    }
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
    fprintf(stderr, " -H [threshold]  High threshold in milliseconds (when dark). Default %d\n", LDR_DEFAULT_HIGH_THRESHOLD);
    fprintf(stderr, " -L [threshold]  Low threshold in milliseconds (when bright). Default %d\n", LDR_DEFAULT_LOW_THRESHOLD);
    fprintf(stderr, " -D [duration]   High threshold debounce duration in seconds. Default %d\n", LDR_DEFAULT_HIGH_DURATION_MS/1000);
    fprintf(stderr, " -d [duration]   Low threshold debounce duration in seconds. Default %d\n", LDR_DEFAULT_LOW_DURATION_MS/1000);
    fprintf(stderr, " -X [command]    Command to run when dark\n");
    fprintf(stderr, " -x [command]    Command to run when bright\n");
    fprintf(stderr, " -b              Run in the background\n");
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
    struct ldr_sensor_t ldr;
    int ldr_gpio = -1;
    unsigned int high_threshold = LDR_DEFAULT_HIGH_THRESHOLD;
    unsigned int low_threshold = LDR_DEFAULT_LOW_THRESHOLD;
    unsigned int complete_darkness_threshold = LDR_DEFAULT_COMPLETE_DARKNESS_THRESHOLD;
    unsigned int high_threshold_duration_ms = LDR_DEFAULT_HIGH_DURATION_MS;
    unsigned int low_threshold_duration_ms = LDR_DEFAULT_LOW_DURATION_MS;
    unsigned int complete_darkness_duration_ms = LDR_DEFAULT_COMPLETE_DARKNESS_DURATION_MS;
    struct trigger_action_t action;
    int opt;
    int ret = 0;
    int i;
    char *gpio_str;

    trigger_action_init(&action);

    progname = argv[0];

    while (((opt = getopt(argc, argv, "g:G:H:L:D:d:n:x:X:bvh")) != -1))
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
                    if (gpio_in_list(&action.gpio_list_head, output_gpio)) {
                        LOG_ERROR("Error: output GPIO pin %d already specified\n", output_gpio);
                        exit(EXIT_FAILURE);
                    }
                    if (inverted)
                        LOG_VERBOSE("Output GPIO pin %d is inverted\n", output_gpio);
                    LOG_VERBOSE("Adding output GPIO pin %d to list\n", output_gpio);
                    if (append_output_gpio(&action.gpio_list_head, output_gpio, inverted)) {
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

            case 'D':
                high_threshold_duration_ms = atoi(optarg);
                if (high_threshold_duration_ms < 0) {
                    LOG_ERROR("Error: Invalid high threshold debounce duration %d\n", high_threshold_duration_ms);
                    exit(EXIT_FAILURE);
                }
                high_threshold_duration_ms = high_threshold_duration_ms * 1000;
                break;

            case 'd':
                low_threshold_duration_ms = atoi(optarg);
                if (low_threshold_duration_ms < 0) {
                    LOG_ERROR("Error: Invalid low threshold debounce duration %d\n", low_threshold_duration_ms);
                    exit(EXIT_FAILURE);
                }
                low_threshold_duration_ms = low_threshold_duration_ms * 1000;
                break;

            case 'X':
                action.cmd_dark = optarg;
                switch (wordexp(action.cmd_dark, &action.cmd_dark_exp_result, WRDE_NOCMD))
                {
                    case 0:
                        break;
                    case WRDE_BADCHAR:
                        LOG_ERROR("Error: bad char: %s\n", action.cmd_dark);
                        exit(EXIT_FAILURE);
                    case WRDE_CMDSUB:
                        LOG_ERROR("Error: command substitution not allowed: %s\n", action.cmd_dark);
                        exit(EXIT_FAILURE);
                    case WRDE_NOSPACE:
                        LOG_ERROR("Error: failed to allocate memory: %s\n", action.cmd_dark);
                        exit(EXIT_FAILURE);
                    case WRDE_SYNTAX:
                        LOG_ERROR("Error: syntax error: %s\n", action.cmd_dark);
                        exit(EXIT_FAILURE);
                    default:
                        LOG_ERROR("Error: unknown error: %s\n", action.cmd_dark);
                        exit(EXIT_FAILURE);
                }
                break;
            case 'x':
                action.cmd_bright = optarg;
                switch (wordexp(action.cmd_bright, &action.cmd_bright_exp_result, WRDE_NOCMD))
                {
                    case 0:
                        break;
                    case WRDE_BADCHAR:
                        LOG_ERROR("Error: bad char: %s\n", action.cmd_bright);
                        exit(EXIT_FAILURE);
                    case WRDE_CMDSUB:
                        LOG_ERROR("Error: command substitution not allowed: %s\n", action.cmd_bright);
                        exit(EXIT_FAILURE);
                    case WRDE_NOSPACE:
                        LOG_ERROR("Error: failed to allocate memory: %s\n", action.cmd_bright);
                        exit(EXIT_FAILURE);
                    case WRDE_SYNTAX:
                        LOG_ERROR("Error: syntax error: %s\n", action.cmd_bright);
                        exit(EXIT_FAILURE);
                    default:
                        LOG_ERROR("Error: unknown error: %s\n", action.cmd_bright);
                        exit(EXIT_FAILURE);
                }
                break;
            case 'b': daemonize = 1; break;
            case 'v': new_log_level++; set_log_level(new_log_level); break;
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


    if (ldr_gpio < 0) {
        LOG_ERROR("Error: LDR GPIO pin not specified\n");
        exit(EXIT_FAILURE);
    }
    if (gpio_in_list(&action.gpio_list_head, ldr_gpio)) {
        LOG_ERROR("Error: LDR GPIO pin %d is used as output\n", ldr_gpio);
        exit(EXIT_FAILURE);
    }

    if (high_threshold <= low_threshold) {
        LOG_ERROR("Error: high threshold must be greater than low threshold\n");
        exit(EXIT_FAILURE);
    }
    if (complete_darkness_threshold <= high_threshold) {
        LOG_ERROR("Error: complete darkness threshold must be greater than high threshold\n");
        exit(EXIT_FAILURE);
    }


    if (daemonize)
    {
        pid_t pid, sid;
        pid = fork();
        if (pid < 0)
        {
            LOG_ERROR("Error: fork failed\n");
            exit(EXIT_FAILURE);
        }
        // If we got a good PID, then we can exit the parent process.
        if (pid > 0)
            exit(EXIT_SUCCESS);

        // we are now in child process

        // Change the file mode mask
        //umask(0);

        // Create a new SID for the child process
        sid = setsid();
        if (sid < 0)
        {
            LOG_ERROR("Error: set SID failed\n");
            exit(EXIT_FAILURE);
        }
    }


    signal(SIGINT, handle_terminate_signal);
    signal(SIGTERM, handle_terminate_signal);

    if (ldr_init(&ldr, ldr_gpio)) {
        LOG_ERROR("Error: Failed to initialize LDR GPIO pin\n");
        ret = -1;
        goto clean_up;
    }
    ldr_configure(&ldr, high_threshold, low_threshold,
                  complete_darkness_threshold, high_threshold_duration_ms,
                  low_threshold_duration_ms, complete_darkness_duration_ms);


    ldr_register_callback(&ldr, ldr_trigger_cb, &action);

    // init output GPIO pins
    if (init_all_output_gpio(&action.gpio_list_head)) {
        LOG_ERROR("Error: Failed to initialize output GPIO pins\n");
        ret = -1;
        goto clean_up;
    }


    while (!terminate) {
        ldr_read_once(&ldr);
    }



clean_up:
    trigger_action_cleanup(&action);
    ldr_cleanup(&ldr);

    exit(ret);
}
