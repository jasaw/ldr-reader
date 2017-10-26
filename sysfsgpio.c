/*
 *    Filename: sysfsgpio.c
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

#include "utils.h"
#include "sysfsgpio.h"

#define PIN_BUFFER_SIZE 3
#define MAX_PATH_BUFFER 40


int gpio_export(int pin)
{
	char buffer[PIN_BUFFER_SIZE];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		LOG_ERROR("Failed to open gpio export for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, PIN_BUFFER_SIZE, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

int gpio_unexport(int pin)
{
	char buffer[PIN_BUFFER_SIZE];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		LOG_ERROR("Failed to open gpio unexport for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, PIN_BUFFER_SIZE, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

int gpio_open_direction(int pin)
{
	char path[MAX_PATH_BUFFER];
	int fd;

	snprintf(path, MAX_PATH_BUFFER, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_RDWR);
	if (-1 == fd)
		LOG_ERROR("Failed to open gpio direction!\n");
	return fd;
}

int gpio_direction(int pin, int dir)
{
	static const char s_directions_str[]  = "in\0out";

	int fd = gpio_open_direction(pin);
	if (fd == -1)
		return(-1);

	if (-1 == write(fd, &s_directions_str[GPIO_IN == dir ? 0 : 3], GPIO_IN == dir ? 2 : 3)) {
		LOG_ERROR("Failed to set gpio direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

int gpio_open_edge(int pin)
{
	char path[MAX_PATH_BUFFER];
	int fd;

	snprintf(path, MAX_PATH_BUFFER, "/sys/class/gpio/gpio%d/edge", pin);
	fd = open(path, O_RDWR);
	if (-1 == fd)
		LOG_ERROR("Failed to open gpio edge!\n");
	return fd;
}

int gpio_edge(int pin, int edge)
{
	static const char s_edge_str[]  = "none\0rising\0falling\0both";

	int fd;
	int s_edge_index;
	int s_edge_length;

	switch (edge)
	{
		case GPIO_EDGE_RISING:  s_edge_index = 5;  s_edge_length = 6; break;
		case GPIO_EDGE_FALLING: s_edge_index = 12; s_edge_length = 7; break;
		case GPIO_EDGE_BOTH:    s_edge_index = 20; s_edge_length = 4; break;
		default:                s_edge_index = 0;  s_edge_length = 4;
	}

	fd = gpio_open_edge(pin);
	if (fd == -1)
		return(-1);

	if (-1 == write(fd, &s_edge_str[s_edge_index], s_edge_length)) {
		LOG_ERROR("Failed to set gpio edge!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

int gpio_open_active_low(int pin)
{
	char path[MAX_PATH_BUFFER];
	int fd;

	snprintf(path, MAX_PATH_BUFFER, "/sys/class/gpio/gpio%d/active_low", pin);
	fd = open(path, O_RDWR);
	if (-1 == fd)
		LOG_ERROR("Failed to open gpio active_low!\n");
	return fd;
}

int gpio_active_low(int pin, int active_high_low)
{
	static const char s_invert_str[] = "01";

	int fd;

	fd = gpio_open_active_low(pin);
	if (fd == -1)
		return(-1);

	if (1 != write(fd, &s_invert_str[GPIO_ACTIVE_HIGH == active_high_low ? 0 : 1], 1)) {
		LOG_ERROR("Failed to write gpio active_low!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

int gpio_open_value(int pin)
{
	char path[MAX_PATH_BUFFER];
	int fd;

	snprintf(path, MAX_PATH_BUFFER, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDWR);
	if (-1 == fd)
		LOG_ERROR("Failed to open gpio value!\n");
	return fd;
}

int gpio_read(int pin)
{
	char value_str[3];
	int fd;

	fd = gpio_open_value(pin);
	if (fd == -1)
		return(-1);

	if (-1 == read(fd, value_str, 3)) {
		LOG_ERROR("Failed to read gpio value!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

int gpio_write(int pin, int value)
{
	static const char s_values_str[] = "01";

	int fd;

	fd = gpio_open_value(pin);
	if (fd == -1)
		return(-1);

	if (1 != write(fd, &s_values_str[GPIO_LOW == value ? 0 : 1], 1)) {
		LOG_ERROR("Failed to write gpio value!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

// set timeout_ms to -1 to block indefinitely
// set timeout_ms to return immediately
// returns positive value if successful.
// returns zero if timed out.
// returns -1 if error.
int gpio_wait_for_interrupt_fd(int fd, int timeout_ms)
{
	char value_str[3];
	struct pollfd pfd;
	int ret;

	pfd.fd = fd;
	pfd.events = POLLPRI;

	// consume any prior interrupt
	lseek(fd, 0, SEEK_SET);
	read(fd, value_str, sizeof(value_str));

	// wait for interrupt
	ret = poll(&pfd, 1, timeout_ms);
	if (ret < 0) {
		if (errno != EINTR)
			LOG_ERROR("Error: poll failed: %s\n", strerror(errno));
	}

	// consume interrupt
	lseek(fd, 0, SEEK_SET);
	read(fd, value_str, sizeof(value_str));

	return ret;
}

int gpio_wait_for_interrupt(int pin, int timeout_ms)
{
	int fd;
	int ret;

	fd = gpio_open_value(pin);
	if (fd == -1)
		return(-1);

	ret = gpio_wait_for_interrupt_fd(fd, timeout_ms);

	close(fd);

	return ret;
}

int gpio_write_string(int fd, const char *str, const char *filename)
{
	if (-1 == write(fd, str, strlen(str))) {
		LOG_ERROR("Failed to set gpio %s!\n", filename);
		return(-1);
	}
	fsync(fd);
	return 0;
}
