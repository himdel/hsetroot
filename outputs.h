#ifndef __OUTPUTS_H__
#define __OUTPUTS_H__

typedef struct {
  int w, h;
  int x, y;
} OutputInfo;

OutputInfo *outputs_list(int *count_out);
void outputs_free(OutputInfo *);

#endif	// __OUTPUTS_H__
