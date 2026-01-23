#include <stdint.h>
#include <stdio.h>

#include "./src/cli/cli.h"

int main(int argc, char** argv)
{
  Cli_h cli ={0};
  int32_t master_id =0;
  int32_t slaves_id=0;

  if (argc < 3)
  {
    printf("usage cli [can_node] [master_id] [slaves_id]\n");
    return -1;
  }

  sscanf(argv[2], "%d", &master_id); 
  sscanf(argv[3], "%d", &slaves_id); 

  if (cli_init(&cli, argv[1], (uint16_t) master_id,(uint16_t) slaves_id))
  {
    fprintf(stderr,"failed init cli\n");
    return -2;
  }

  if (cli_start(&cli))
  {
    fprintf(stderr,"cli crushed\n");
    return -3;
  }

  return 0;
}
