#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	char name[512];
	char num[512];
	char help[512];
	char def[512];
	char args[512];
	char buf[512];
	char *defines[512];
  	int i = 0, max;

	printf("/* this file is auto generated, edit textevents.in instead! */\n\nconst struct text_event te[] = {\n");
	while(fgets(name, sizeof(name), stdin))
	{
		name[strlen(name)-1] = 0;
		fgets(num, sizeof(num), stdin);
		num[strlen(num)-1] = 0;
		fgets(help, sizeof(help), stdin);
		help[strlen(help)-1] = 0;
		fgets(def, sizeof(def), stdin);
		def[strlen(def)-1] = 0;
		fgets(args, sizeof(args), stdin);
		args[strlen(args)-1] = 0;
		fgets(buf, sizeof(buf), stdin);

		if (args[0] == 'n')
			printf("\n{\"%s\", %s, %d, \n\"%s\"},\n",
							 name, help, atoi(args+1) | 128, def);
		else
			printf("\n{\"%s\", %s, %d, \nN_(\"%s\")},\n",
							 name, help, atoi(args), def);
		defines[i] = strdup (num);
		i++;
	}

	printf("};\n");
	
	fprintf(stderr, "/* this file is auto generated, edit textevents.in instead! */\n\nenum\n{\n");
	max = i;
	i = 0;
	while (i < max)
	{
		if (i + 1 < max)
		{
			fprintf(stderr, "\t%s,\t\t%s,\n", defines[i], defines[i+1]);
			i++;
		} else
			fprintf(stderr, "\t%s,\n", defines[i]);
		i++;
	}
	fprintf(stderr, "\tNUM_XP\n};\n");

	return 0;
}
