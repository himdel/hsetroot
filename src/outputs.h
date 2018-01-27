#ifndef __OUTPUTS_H__
#define __OUTPUTS_H__

typedef struct {
  int idx;
  char* name;
  int w, h;
  int x, y;
} OutputInfo;

typedef struct {
  int noutputs;
  OutputInfo* infos;
} Outputs;

void outputs_set(Outputs*);
void outputs_print(OutputInfo);
void outputs_free(Outputs*);

#endif	// __OUTPUTS_H__
