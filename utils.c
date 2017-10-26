/*
 *    Filename: utils.c
 * Description: utils.
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
#include <fcntl.h>

#include "utils.h"


log_level_t _log_level = LOG_INFO;

void set_log_level(log_level_t new_log_level)
{
    _log_level = new_log_level;
}



void print_hex_bytes(FILE *stream, const uint8_t *buffer, int len)
{
    int i;
    for (i = 0; i < len; i++)
        fprintf(stream, "%02x ", buffer[i]);
    fprintf(stream, "\n");
}

void udelay(unsigned s)
{
    struct timeval tv;
    tv.tv_sec = s/1000000;
    tv.tv_usec = s%1000000;
    select (0,NULL,NULL,NULL, &tv);
}
