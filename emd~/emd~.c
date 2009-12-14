#include "ext.h"
#include "z_dsp.h"
#include "EmpiricalModeDecomposition.h"

void *emd_class;

typedef struct _emd {
    t_pxobject x_obj;
	emdData emd;
	long addvSize;
	void** addv;
} t_emd;

void *emd_new(long order, long iterations, long locality);
void emd_free(t_emd* x);
void emd_locality(t_emd* x, long locality);
void emd_iterations(t_emd* x, long iterations);
t_int* emd_perform(t_int* w);

void emd_dsp(t_emd* x, t_signal** sp, short* count);
void emd_assist(t_emd* x, void* b, long m, long a, char* s);

void main(void) {
    setup(
		(t_messlist **) &emd_class,
		(method) emd_new,
		(method) emd_free,
		(short) sizeof(t_emd),
		(method) NULL,
		A_DEFLONG, A_DEFLONG, A_DEFLONG, 0);
    
    addmess((method) emd_dsp, "dsp", A_CANT, 0);
    addmess((method) emd_assist, "assist", A_CANT, 0);
	addmess((method) emd_locality, "locality", A_DEFLONG, 0);
	addmess((method) emd_iterations, "iterations", A_DEFLONG, 0);
    dsp_initclass();
}

void emd_assist(t_emd *x, void *box, long direction, long position, char *text) {
	if (direction == ASSIST_INLET)
		sprintf(text, "(Signal) to be decomposed");
	else
		sprintf(text, "(Signal) IMF %d", position);
}

void *emd_new(long order, long iterations, long locality) {
	int i;
    t_emd* x = (t_emd *) newobject(emd_class);
	emdSetup(&(x->emd), order, iterations, locality);
	x->addvSize = 2 + order;
	x->addv = cnew(void*, x->addvSize);
    dsp_setup((t_pxobject*) x, 1);
	for(i = 0; i < order; i++)
		outlet_new((t_pxobject*) x, "signal");
	//post("emd_new");
    return (x);
}

void emd_free(t_emd* x) {
	dsp_free((t_pxobject*) x);
	emdClear(&(x->emd));
	cdelete(x->addv);
	//post("emd_free");
}

void emd_dsp(t_emd *x, t_signal **sp, short *count) {
	int i;
	int before = x->emd.size;
	int after = sp[0]->s_n;
	if(before != after)
		emdResize(&(x->emd), after);
	x->addv[0] = x;
	for(i = 0; i < 1 + x->emd.order; i++)
		x->addv[i + 1] = sp[i]->s_vec;
	dsp_addv(emd_perform, x->addvSize, x->addv);
	//post("emd_dsp");
}

t_int* emd_perform(t_int* w) {
	int i;
	t_emd* x = (t_emd*) w[1];
	emdData* emd = &(x->emd);
	long n = emd->size;
	float* in = (float*) w[2];
	emdDecompose(emd, in);
	for(i = 0; i < emd->order; i++)
		memcpy((float*) (w[3 + i]), emd->imfs[i], n * sizeof(float));
	return w + x->addvSize + 1;
}

void emd_locality(t_emd *x, long locality) {
	x->emd.locality = locality;
}

void emd_iterations(t_emd *x, long iterations) {
	x->emd.iterations = iterations;
}