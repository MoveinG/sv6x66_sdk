#include <stdio.h>
#include "osal.h"
#include "gpio/drv_gpio.h"

#if !defined(TY_CLOUD_EN)
typedef struct
{
	int32_t		ionum;
	gpio_pin_t	iopin[4];
} ctrl_group;

typedef struct
{
	int32_t group_num;
	ctrl_group *group;
} gpio_test_table;

static ctrl_group groups[] = 
{
	// group 0
	{
		.ionum  = 2,
		.iopin = {GPIO_01, GPIO_13}
	},
	// group 1
	{
		.ionum  = 2,
		.iopin = {GPIO_00, GPIO_12}
	},
};

static gpio_test_table test_table = {
	.group_num = 2,
	.group = groups
};

bool gpio_test(void)
{
	int idx,i,j;

	for(idx = 0; idx < test_table.group_num; idx++)
	{
		for(i = 0; i < test_table.group[idx].ionum; i++)
		{
            //set io direction
            for(j = 0; j < test_table.group[idx].ionum; j++)
			{
				drv_gpio_set_mode(test_table.group[idx].iopin[j], PIN_MODE_GPIO);

				if(i == j)
					drv_gpio_set_dir(test_table.group[idx].iopin[j], GPIO_DIR_OUT);
				else
					drv_gpio_set_dir(test_table.group[idx].iopin[j], GPIO_DIR_IN);
            }

            // write 1
            drv_gpio_set_logic(test_table.group[idx].iopin[i], GPIO_LOGIC_HIGH);

            for(j = 0; j < test_table.group[idx].ionum; j++)
			{
				if(i != j)
				{
					if(drv_gpio_get_logic(test_table.group[idx].iopin[j]) != GPIO_LOGIC_HIGH)
					{
						printf("gpio(%d:%d)1 test err\n", idx, j);
						return false;
					}
				}
			}

            // write 0
            drv_gpio_set_logic(test_table.group[idx].iopin[i], GPIO_LOGIC_LOW);

            for(j = 0; j < test_table.group[idx].ionum; j++)
			{
				if(i != j)
				{
					if(drv_gpio_get_logic(test_table.group[idx].iopin[j]) != GPIO_LOGIC_LOW)
					{
						printf("gpio(%d:%d)0 test err\n", idx, j);
						return false;
					}
				}
			}
		}
	}

	printf("gpio test succ\n");
	return true;
}
#endif

