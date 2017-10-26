/*
 *    Filename: utils.h
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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdint.h>

typedef enum
{
    LOG_INFO = 0,
    LOG_VERBOSE,
    LOG_DEBUG
} log_level_t;


#define LOG_ERROR(text, var...)     fprintf(stderr, text, ##var)
#define LOG_INFO(text, var...)      fprintf(stdout, text, ##var)
#define LOG_VERBOSE(text, var...)   if (_log_level >= LOG_VERBOSE) fprintf(stdout, text, ##var)
#define LOG_DEBUG(text, var...)     if (_log_level >= LOG_DEBUG) fprintf(stdout, text, ##var)
#define LOG_ERROR_HEX_BYTES(buf, len)   print_hex_bytes(stderr, buf, len)
#define LOG_INFO_HEX_BYTES(buf, len)    print_hex_bytes(stdout, buf, len)
#define LOG_VERBOSE_HEX_BYTES(buf, len) if (_log_level >= LOG_VERBOSE) print_hex_bytes(stdout, buf, len)
#define LOG_DEBUG_HEX_BYTES(buf, len)   if (_log_level >= LOG_DEBUG) print_hex_bytes(stdout, buf, len)

void set_log_level(log_level_t new_log_level);

void print_hex_bytes(FILE *stream, const uint8_t *buffer, int len);

void udelay(unsigned s);


extern log_level_t _log_level;

#endif // _UTILS_H_
