// tabread2d~

/* 
This software is copyrighted by Miller Puckette and others.  The following
terms (the "Standard Improved BSD License") apply to all files associated with
the software unless explicitly disclaimed in individual files:

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above  
   copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided
   with the distribution.
3. The name of the author may not be used to endorse or promote
   products derived from this software without specific prior 
   written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

// Cyrille Henry 09 2024

#include "m_pd.h"

/******************** tabread2d~ ***********************/

static t_class *tabread2d_tilde_class;

typedef struct _tabread2d_tilde
{
    t_object x_obj;
    int x_npoints;
    int x_npointsX, x_npointsY;
    t_word *x_vec;
    t_symbol *x_arrayname;
    t_inlet *x_in2;
    t_outlet *x_out;
    t_float f;
} t_tabread2d_tilde;


void tabread2d_tilde_set(t_tabread2d_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name)
            pd_error(x, "tabread2d~: %s: no such array", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatwords(a, &x->x_npoints, &x->x_vec))
    {
        pd_error(x, "%s: bad template for tabread2d~", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else garray_usedindsp(a);
    if (x->x_npoints < x->x_npointsX * x->x_npointsY) {
   		pd_error(x, "not enought points for %d*%d interpolation", x->x_npointsX, x->x_npointsY);
   		// TODO : do somthing to prevent crash 
   	}	
}


void *tabread2d_tilde_new(t_symbol *s, t_floatarg sizex, t_floatarg sizey)
{
    t_tabread2d_tilde *x = (t_tabread2d_tilde *)pd_new(tabread2d_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    x->x_out = outlet_new(&x->x_obj, gensym("signal"));
    x->x_in2 = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    x->x_npointsX = sizex;
    x->x_npointsY = sizey;
    tabread2d_tilde_set(x, x->x_arrayname);
    
    return (void *)x;
}

float tabread2d_read (t_tabread2d_tilde *x, int X, int Y) {
	int index = X + Y * x->x_npointsX;
	return x->x_vec[index].w_float;
}

t_int *tabread2d_tilde_perform(t_int *w)
{
    t_tabread2d_tilde *x = (t_tabread2d_tilde *)(w[1]);
    t_sample *in1 = (t_sample *)(w[2]);
    t_sample *in2 = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    int n = (int)(w[5]);    
    t_word *buf = x->x_vec;
    int i;

    if (!buf) {
    	while (n--) *out++ = 0;
    	return (w+6);
    }
    
    for (i = 0; i < n; i++)
    {
        t_sample findexX = *in1++;
        t_sample findexY = *in2++;
        int iindexX = findexX;
        int iindexY = findexY;
        float fractX = findexX - iindexX;
        float fractY = findexY - iindexY;
        float fractX2 = fractX * fractX;
        float fractX3 = fractX2 * fractX;
        float fractY2 = fractY * fractY;
        float fractY3 = fractY2 * fractY; 
	 
		float C00 = tabread2d_read(x, iindexX-1, iindexY-1 );
		float C10 = tabread2d_read(x, iindexX,   iindexY-1 );
		float C20 = tabread2d_read(x, iindexX+1, iindexY-1 );
		float C30 = tabread2d_read(x, iindexX+2, iindexY-1 );
		float C01 = tabread2d_read(x, iindexX-1, iindexY );
		float C11 = tabread2d_read(x, iindexX,   iindexY );
		float C21 = tabread2d_read(x, iindexX+1, iindexY );
		float C31 = tabread2d_read(x, iindexX+2, iindexY );
		float C02 = tabread2d_read(x, iindexX-1, iindexY+1);
		float C12 = tabread2d_read(x, iindexX,   iindexY+1 );
		float C22 = tabread2d_read(x, iindexX+1, iindexY+1 );
		float C32 = tabread2d_read(x, iindexX+2, iindexY+1 );
		float C03 = tabread2d_read(x, iindexX-1, iindexY+2 );
		float C13 = tabread2d_read(x, iindexX,   iindexY+2 );
		float C23 = tabread2d_read(x, iindexX+1, iindexY+2 );
		float C33 = tabread2d_read(x, iindexX+2, iindexY+2 );
		// TODO : min/ max sur les index
		
		float w0 = C11;
		float w1 = C21;
		float w2 = C12;
		float w3 = C22;
		// x derivative
		float x0 = (C21 - C01) / 2.;
		float x1 = (C31 - C11) / 2.;
		float x2 = (C22 - C02) / 2.;
		float x3 = (C32 - C12) / 2.;
		// y derivative
		float y0 = (C12 - C10) / 2.;
		float y1 = (C22 - C20) / 2.;
		float y2 = (C13 - C11) / 2.;
		float y3 = (C23 - C21) / 2.;
		// xy derivative
		float z0 = (C22 - C00) / 2.;
		float z1 = (C32 - C10) / 2.;
		float z2 = (C23 - C01) / 2.;
		float z3 = (C33 - C11) / 2.;

		float a00 = w0;
		float a01 = y0;
		float a02 = -3.*w0 + 3.*w2 -2.*y0 - y2;
		float a03 = 2.*w0 - 2.*w2 + y0 + y2;
		float a10 = x0;
		float a11 = z0;
		float a12 = -3.*x0 + 3.*x2 - 2.*z0 - z2;
		float a13 = 2.*x0 - 2.*x2 + z0 + z2;
		float a20 = -3.*w0 + 3.*w1 - 2.*x0 - x1;
		float a21 = -3.*y0 + 3.*y1 - 2.*z0 - z1;
		float a22 = 9.*w0 - 9.*w1 - 9.*w2 + 9.*w3 + 6.*x0 + 3.*x1 + -6.*x2 - 3.*x3 + 6.*y0 - 6.*y1 + 3.*y2 - 3.*y3 + 4.*z0 + 2.*z1 + 2.*z2 + z3;
		float a23 = -6.*w0 + 6.*w1 + 6.*w2 - 6.*w3 -4.*x0 - 2.*x1 + 4.*x2 + 2.*x3 -3.*y0 + 3.*y1 - 3.*y2 + 3.*y3 + -2.*z0 - z1 - 2.*z2 - z3;
		float a30 = 2.*w0 - 2.*w1 + x0 + x1;
		float a31 = 2.*y0 - 2.*y1 + z0 + z1;
		float a32 = -6.*w0 + 6.*w1 + 6.*w2 -6.*w3 -3.*x0 - 3.*x1 + 3.*x2 + 3.*x3 -4.*y0 + 4.*y1 - 2.*y2 + 2.*y3 + -2.*z0 - 2.*z1 - z2 - z3;
		float a33 = 4.*w0 - 4.*w1 - 4.*w2 + 4.*w3 + 2.*x0 + 2.*x1 + -2.*x2 - 2.*x3 + 2.*y0 - 2.*y1 + 2.*y2 - 2.*y3 + z0 + z1 + z2 + z3;


		float interpolation = a00;
		interpolation += a01 * fractY;
		interpolation += a02 * fractY2;
		interpolation += a03 * fractY3;
		interpolation += a10 * fractX;
		interpolation += a11 * fractX * fractY;
		interpolation += a12 * fractX * fractY2;
		interpolation += a13 * fractX * fractY3;
		interpolation += a20 * fractX2;
		interpolation += a21 * fractX2 * fractY;
		interpolation += a22 * fractX2 * fractY2;
		interpolation += a23 * fractX2 * fractY3;
		interpolation += a30 * fractX3;
		interpolation += a31 * fractX3 * fractY;
		interpolation += a32 * fractX3 * fractY2;
		interpolation += a33 * fractX3 * fractY3;

		*out++ =  interpolation;
    }
    return (w+6);

}

void tabread2d_tilde_dsp(t_tabread2d_tilde *x, t_signal **sp)
{
    dsp_add(tabread2d_tilde_perform, 5, x,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

void tabread2d_tilde_free(t_tabread2d_tilde *x)
{
  inlet_free(x->x_in2);
  outlet_free(x->x_out);
}

void tabread2d_tilde_setup(void)
{
    tabread2d_tilde_class = class_new(gensym("tabread2d~"),
        (t_newmethod)tabread2d_tilde_new, 
        (t_method)tabread2d_tilde_free,
        sizeof(t_tabread2d_tilde), 
        CLASS_DEFAULT, 
        A_DEFSYM, A_DEFFLOAT, A_DEFFLOAT, 0);
        
    CLASS_MAINSIGNALIN(tabread2d_tilde_class, t_tabread2d_tilde, f);
    class_addmethod(tabread2d_tilde_class, (t_method)tabread2d_tilde_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(tabread2d_tilde_class, (t_method)tabread2d_tilde_set,
        gensym("set"), A_SYMBOL, 0);
    class_addmethod(tabread2d_tilde_class, (t_method)tabread2d_tilde_set,
        gensym("size"), A_FLOAT, A_FLOAT, 0);
        
}
