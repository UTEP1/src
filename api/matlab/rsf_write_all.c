/* Write complete RSF file, both header and data, in one call.
 *
 * MATLAB usage:
 *   rsf_write_all(file,data[,dalta[,origin[,label[,unit]]]])
 *
 * Written by Henryk Modzelewski, UBC EOS SLIM
 * Created February 2012
 */
/*
  Copyright (C) 2012 The University of British Columbia at Vancouver
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <mex.h>
#include <string.h>
#include <rsf.h>

void mexFunction(int nlhs, mxArray *plhs[], 
		 int nrhs, const mxArray *prhs[])
{
    int strlen, status, argc=2, i, ndim, odim, len;
    const int *dim=NULL;
    size_t nbuf = BUFSIZ, nd, j;
    char *strtag=NULL, *argv[] = {"matlab","-"}, *par=NULL, *filename=NULL;
    double *dr=NULL, *di=NULL;
    double *ddlt=NULL, *dorg=NULL;
    mxArray *pca;
    float *p=NULL;
    sf_complex *c=NULL;
    char buf[BUFSIZ], key[8];
    sf_file file=NULL;
    
    /* Check for proper number of arguments. */
    if (nrhs < 2 || nrhs > 6)
	 mexErrMsgTxt("2 to 6 inputs required:\n\tfile,data[,dalta[,origin[,label[,unit]]]]");
    if (nlhs != 0)
	 mexErrMsgTxt("This function has no outputs");

    /* File name must be a string. */
    if (!(mxIsChar(prhs[0])&&mxGetM(prhs[0])==1)) mexErrMsgTxt("File name must be a string.");
    /* Get the length of the input string. */
    strlen = mxGetN(prhs[0]) + 1;
    /* Allocate memory for input string. */
    filename = mxCalloc(strlen, sizeof(char));
    /* Copy the string data from prhs[0] into a C string. */
    status = mxGetString(prhs[0], filename, strlen);
    if (status != 0) 
	mexWarnMsgTxt("Not enough space. String is truncated.");

    /* Data must be a double array. */
    if (!mxIsDouble(prhs[1])) mexErrMsgTxt("Data must be a double array.");
    /* get data dimensions */
    ndim=mxGetNumberOfDimensions(prhs[1]);
    dim=mxGetDimensions(prhs[1]);
    /* get data size */
    nd = mxGetNumberOfElements(prhs[1]);

    /* Deltas must be a double row vector. */
    if (nrhs > 2) {
	if (!mxIsDouble(prhs[2])) mexErrMsgTxt("Delta must be double.");
	if (mxGetM(prhs[2]) != 1) mexErrMsgTxt("Deltas must be a row vector.");
	odim = mxGetN(prhs[2]);
	ddlt = mxGetPr(prhs[2]);
	if (odim != ndim) mexErrMsgTxt("Deltas has wrong number of elements.");
    }

    /* Origins must be a double row vector. */
    if (nrhs > 3) {
	if (!mxIsDouble(prhs[3])) mexErrMsgTxt("Delta must be double.");
	if (mxGetM(prhs[3]) != 1) mexErrMsgTxt("Deltas must be a row vector.");
	odim = mxGetN(prhs[3]);
	dorg = mxGetPr(prhs[3]);
	if (odim != ndim) mexErrMsgTxt("Deltas has wrong number of elements.");
    }

    /* Labels must be a cell array of strings. */
    if (nrhs > 4) {
	if (!mxIsCell(prhs[4])) mexErrMsgTxt("Labels must be a cell array.");
	odim = mxGetNumberOfElements(prhs[4]);
	if (odim != ndim) mexErrMsgTxt("Labels has wrong number of elements.");
    }

    /* Units must be a cell array of strings. */
    if (nrhs > 5) {
	if (!mxIsCell(prhs[5])) mexErrMsgTxt("Units must be a cell array.");
	odim = mxGetNumberOfElements(prhs[5]);
	if (odim != ndim) mexErrMsgTxt("Units has wrong number of elements.");
    }

    sf_init(argc,argv);
    file = sf_output(filename);
    sf_setformat(file,mxIsComplex(prhs[1])?"native_complex":"native_float");

    /* Write header */
    for (i=0; i < ndim; i++) {
	/* sizes */
        sprintf(key,"n%d",i+1);
        sf_putint(file,key,(int)dim[i]);
	/* deltas */
        sprintf(key,"d%d",i+1);
        if (nrhs > 2) sf_putfloat(file,key,(float)ddlt[i]);
        else sf_putfloat(file,key,1.);
	/* origins */
        sprintf(key,"o%d",i+1);
        if (nrhs > 3) sf_putfloat(file,key,(float)dorg[i]);
        else sf_putfloat(file,key,0.);
	/* labels */
	if (nrhs > 4) {
	    pca = mxGetCell(prhs[4], i);
	    if (!mxIsChar(pca)) mexErrMsgTxt("Label must be a string.");
	    strlen = mxGetN(pca) + 1;
	    strtag = mxCalloc(strlen, sizeof(char));
	    status = mxGetString(pca, strtag, strlen);
	    sprintf(key,"label%d",i+1);
	    if (strlen > 1) sf_putstring(file,key,strtag);
	    mxFree(strtag);
	}
	/* units */
	if (nrhs > 5) {
	    pca = mxGetCell(prhs[5], i);
	    if (!mxIsChar(pca)) mexErrMsgTxt("Unit must be a string.");
	    strlen = mxGetN(pca) + 1;
	    strtag = mxCalloc(strlen, sizeof(char));
	    status = mxGetString(pca, strtag, strlen);
	    sprintf(key,"unit%d",i+1);
	    if (strlen > 1) sf_putstring(file,key,strtag);
	    mxFree(strtag);
	}
    }
    
    /* Write data */
    if (mxIsComplex(prhs[1])) {
	/* complex data */
	c = (sf_complex*) buf;

	dr = mxGetPr(prhs[1]);

	/* pointer to imaginary part */
	di = mxGetPi(prhs[1]);
	
	for (j=0, nbuf /= sizeof(sf_complex); nd > 0; nd -= nbuf) {
	    if (nbuf > nd) nbuf=nd;
	    
	    for (i=0; i < nbuf; i++, j++) {
		c[i] = sf_cmplx((float) dr[j],(float) di[j]);
	    }
	    
	    sf_complexwrite(c,nbuf,file);
	}

    } else { 
	/* real data */
	p = (float*) buf;

	dr = mxGetPr(prhs[1]);

	for (j=0, nbuf /= sizeof(float); nd > 0; nd -= nbuf) {
	    if (nbuf > nd) nbuf=nd;
	    
	    for (i=0; i < nbuf; i++, j++) {
		p[i] = (float) dr[j];
	    }
	    
	    sf_floatwrite(p,nbuf,file);
	}
    } 

    sf_fileclose(file);
    sf_close();
}
