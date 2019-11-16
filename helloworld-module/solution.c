#include <linux/module.h>

#include <checker.h>

#define CNT_CALLS_FROM_CHECKER_KO 10
#define ARR_SIZE_FOR_CHECKER 2

struct checker_arr {
    short v[ARR_SIZE_FOR_CHECKER];
    int s;
};

static struct checker_arr checkr_param[CNT_CALLS_FROM_CHECKER_KO] = {
    {{ 0, 0 }, 0 },
    {{ 0, 1 }, 1 },
    {{ 0, 2 }, 2 },
    {{ 0, 3 }, 3 },
    {{ 0, 4 }, 4 },
    {{ 0, 5 }, 5 },
    {{ 0, 6 }, 6 },
    {{ 0, 7 }, 7 },
    {{ 0, 8 }, 8 },
    {{ 0, 9 }, 9 }
};

/*
 * This is the starting point of the kernel module's code execution.
 * There you can allocate some memory, init global variables and so on.
 */
static int solution_init(void)
{
    int i;
    int sum;

    CHECKER_MACRO;

    for (i = 0; i < CNT_CALLS_FROM_CHECKER_KO; ++i) {
        char out[100] = { 0 };

        sum = array_sum(checkr_param[i].v, ARR_SIZE_FOR_CHECKER);
        generate_output(sum, checkr_param[i].v, ARR_SIZE_FOR_CHECKER, out);

        if (sum == checkr_param[i].s)
            printk(KERN_INFO "%s", out);
        else
            printk(KERN_ERR "%s", out);

    }

    return 0;
}


/*
 * This is the exit point of the kernel module. When the module is removed 
 * this function is called to do any sort of cleanup activities and other such
 * stuff.
 * 
 * For example you have made a tree where you keep someinformation - you would 
 * like to place the code for removing the nodes of the tree here.
 */
static void solution_exit(void)
{
    CHECKER_MACRO;
}

module_init(solution_init);
module_exit(solution_exit);

MODULE_DESCRIPTION("Learn Linux Kernel with stepik.org");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
