#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "gpio.h"
#include "oled.h"
#include "yt.h"

#define NPINEO_GPIO_DEV			"gpiochip0"
#define LINES_COUNT			3
#define LINE1_OFFSET			0
#define LINE2_OFFSET			2
#define LINE3_OFFSET			3
#define DISPLAY_OFF_TIMEOUT		30
#define COMMAND_OUTPUT_BUFFER_LEN	(16 + 1)	/* 1 line of text + '\0' */
#define DEBOUNCE_PERIOD_US		100

volatile int exit_thread = 0;

char *get_command_output(char *command)
{
	/* read out command output */
	FILE *fd = popen(command, "r");
	if (fd == NULL)
		return NULL;

	/* put output into a string (static memory) */
	static char buffer[COMMAND_OUTPUT_BUFFER_LEN];
	fgets(buffer, COMMAND_OUTPUT_BUFFER_LEN, fd);

	pclose(fd);

	return buffer;
}

void *draw_screen(void *arg)
{
	int *cmd_index = arg;
	time_t current_time = time(NULL);
	time_t display_refresh_time = 0;
	time_t display_off_time = current_time + DISPLAY_OFF_TIMEOUT;

	while (1) {
		if (exit_thread)
			break;

		usleep(250000);
		current_time = time(NULL);

		if (current_time > display_off_time) {
			/* set display off and exit */
			oled_turn_on_off(0);
			break;
		} else if (current_time > display_refresh_time) {
			/* set display on */
			oled_turn_on_off(1);
			/* draw oled screen */
			switch (*cmd_index) {
			case 0:
				oled_clear_buffer();
				oled_print(get_command_output
					   ("date +%R | awk '{printf \"%15s\", $1}'"),
					   LINE1);
				oled_print("  _  _", LINE1);
				oled_print(" | \\| |___ ___", LINE2);
				oled_print(" | .` / -_) _ \\", LINE3);
				oled_print(" |_|\\_\\___\\___/", LINE4);
				oled_redraw();
				display_refresh_time =
				    current_time + DISPLAY_OFF_TIMEOUT;
				break;
			case 1:{
					char *views, *subs, *videos;
					char buffer[COMMAND_OUTPUT_BUFFER_LEN];
					oled_clear_buffer();
					get_channel_statistics(&views, &subs,
							       &videos);
					oled_print("Your (>)YT stats", LINE1);
					snprintf(buffer,
						 COMMAND_OUTPUT_BUFFER_LEN,
						 "Views: %9s", views);
					oled_print(buffer, LINE2);
					snprintf(buffer,
						 COMMAND_OUTPUT_BUFFER_LEN,
						 "Subs: %10s", subs);
					oled_print(buffer, LINE3);
					snprintf(buffer,
						 COMMAND_OUTPUT_BUFFER_LEN,
						 "Videos: %8s", videos);
					oled_print(buffer, LINE4);
					oled_redraw();
					display_refresh_time =
					    current_time + DISPLAY_OFF_TIMEOUT;
					break;
				}
			case 2:
				oled_clear_buffer();
				oled_print(get_command_output
					   ("hostname -I | awk '{printf \"IP:%13s\", $1}'"),
					   LINE1);
				oled_print(get_command_output
					   ("df -h | awk '$NF==\"/\" {printf \"/:       %2d/%2dGB\", $3, $2}'"),
					   LINE2);
				oled_print(get_command_output
					   ("free -m | awk 'NR==2 {printf \"RAM:   %3s/%3sMB\", $3, $2}'"),
					   LINE3);
				oled_print(get_command_output
					   ("cat /sys/class/thermal/thermal_zone0/temp | awk '{printf \"%15.1fc\", $1/1000}'"),
					   LINE4);
				oled_print(get_command_output
					   ("iostat -c | awk '/[[:digit:]]$/{printf \"Load: %3d%%/\", 100-$NF}'"),
					   LINE4);
				oled_redraw();
				display_refresh_time = current_time + 2;
				break;
			case 3:
				oled_clear_buffer();
				oled_print("   Power off?   ", LINE1);
				oled_print("    -> NO       ", LINE2);
				oled_print("       YES      ", LINE3);
				oled_print("   F3: toggle   ", LINE4);
				oled_redraw();
				display_refresh_time =
				    current_time + DISPLAY_OFF_TIMEOUT;
				break;
			case 4:
				oled_clear_buffer();
				oled_print("   Power off?   ", LINE1);
				oled_print("       NO       ", LINE2);
				oled_print("    -> YES      ", LINE3);
				oled_print("   F1: confirm  ", LINE4);
				oled_redraw();
				display_refresh_time =
				    current_time + DISPLAY_OFF_TIMEOUT;
			}
		}
	}
}

int monitor_gpio(char *dev, int *lines, int num_lines,
		 struct gpio_v2_line_config *config)
{
	struct gpio_v2_line_values values;
	int cmd_index = 0;
	int key1_cmd_index = 1;
	int key2_cmd_index = 2;
	int key3_cmd_index = 3;
	pthread_t draw_thread;
	int fd;
	int rc;

	rc = pthread_create(&draw_thread, NULL, draw_screen, &cmd_index);
	if (rc != 0)
		return EXIT_FAILURE;

	rc = gpio_request_line(dev, lines, num_lines, config, &fd);
	if (rc == EXIT_SUCCESS)
		while (1) {
			struct gpio_v2_line_event event;

			rc = read(fd, &event, sizeof(event));
			if (rc == -1)
				if (errno == -EAGAIN)
					/* nothing available */
					continue;
				else {
					/* failed to read event */
					rc = EXIT_FAILURE;
					break;
				}

			if (rc != sizeof(event)) {
				/* reading event failed */
				rc = EXIT_FAILURE;
				break;
			}

			switch (event.offset) {
			case LINE1_OFFSET:
				cmd_index = key1_cmd_index;
				break;
			case LINE2_OFFSET:
				cmd_index = key2_cmd_index;
				break;
			case LINE3_OFFSET:
				cmd_index = key3_cmd_index;
				break;
			default:
				rc = EXIT_FAILURE;
				goto exit_loop;
			}

			/* starting and terminating the screen threads
			   we always need exactly 1 screen thread! */
			exit_thread = 1;
			rc = pthread_join(draw_thread, NULL);
			if (rc == 0) {
				exit_thread = 0;
				rc = pthread_create(&draw_thread, NULL,
						    draw_screen, &cmd_index);
				if (rc != 0) {
					rc = EXIT_FAILURE;
					goto exit_loop;
				}
			} else {
				rc = EXIT_FAILURE;
				goto exit_loop;
			}

			switch (cmd_index) {
			case 0:
				key1_cmd_index = 1;
				key2_cmd_index = 2;
				key3_cmd_index = 3;
				break;
			case 1:
				key1_cmd_index = 0;
				key2_cmd_index = 2;
				key3_cmd_index = 3;
				break;
			case 2:
				key1_cmd_index = 1;
				key2_cmd_index = 0;
				key3_cmd_index = 3;
				break;
			case 3:
				key1_cmd_index = 1;
				key2_cmd_index = 2;
				key3_cmd_index = 4;
				break;
			case 4:
				key1_cmd_index = 99;
				key2_cmd_index = 4;
				key3_cmd_index = 3;
				break;
			case 99:
				system("shutdown now");
				goto exit_loop;
			default:
				rc = EXIT_FAILURE;
				goto exit_loop;
			}
		}
 exit_loop:

	rc |= gpio_release_line(fd);

	return rc;
}

int main(int argc, char **argv)
{
	struct gpio_v2_line_config config;
	int lines[LINES_COUNT];
	int attr, i;
	int rc;

	oled_init();
	oled_redraw();

	memset(&config, 0, sizeof(config));
	config.flags = GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_EDGE_RISING;
	lines[0] = LINE1_OFFSET;
	lines[1] = LINE2_OFFSET;
	lines[2] = LINE3_OFFSET;

	/* set debounce period for all fn keys */
	attr = config.num_attrs;
	config.num_attrs++;
	for (i = 0; i < LINES_COUNT; i++)
		gpio_set_bit(&config.attrs[attr].mask, i);
	config.attrs[attr].attr.id = GPIO_V2_LINE_ATTR_ID_DEBOUNCE;
	config.attrs[attr].attr.debounce_period_us = DEBOUNCE_PERIOD_US;

	rc = monitor_gpio(NPINEO_GPIO_DEV, lines, LINES_COUNT, &config);
	rc |= oled_turn_on_off(0);

	exit(rc);
}
