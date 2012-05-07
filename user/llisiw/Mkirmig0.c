/* 2-D Post-stack Kirchhoff depth migration. */
/*
  Copyright (C) 2011 University of Texas at Austin
  
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
#include <rsf.h>

#include "kirmig.h"
#include "tinterp.h"

int main(int argc, char* argv[])
{
    int nt, nx, ny, ns, nz, nzx, ix, i, is, ist;
    float *trace, *out, **table, **tablex, *stable, *stablex;
    float ds, s0, x0, y0, dy, s, dx,ti,t0,dt,z0,dz,aal, tx;
    char *unit, *what, *type;
    sf_file inp, mig, tbl, der;

    sf_init (argc,argv);
    inp = sf_input("in");
    tbl = sf_input("table"); /* traveltime table */
    der = sf_input("deriv"); /* source derivative table */
    mig = sf_output("out");

    if (!sf_histint(inp,"n1",&nt)) sf_error("No n1=");
    if (!sf_histint(inp,"n2",&ns)) sf_error("No n2=");

    if (!sf_histfloat(inp,"o1",&t0)) sf_error("No o1=");
    if (!sf_histfloat(inp,"d1",&dt)) sf_error("No d1=");

    if (!sf_histfloat(inp,"o2",&s0)) sf_error("No o2=");
    if (!sf_histfloat(inp,"d2",&ds)) sf_error("No d2=");

    if (!sf_histint(tbl,"n1",&nz)) sf_error("No n1= in table");
    if (!sf_histint(tbl,"n2",&nx)) sf_error("No n2= in table");
    if (!sf_histint(tbl,"n3",&ny)) sf_error("No n3= in table");

    if (!sf_histfloat(tbl,"o1",&z0)) sf_error("No o1= in table");
    if (!sf_histfloat(tbl,"d1",&dz)) sf_error("No d1= in table");
    if (!sf_histfloat(tbl,"o2",&x0)) sf_error("No o2= in table");
    if (!sf_histfloat(tbl,"d2",&dx)) sf_error("No d2= in table");
    if (!sf_histfloat(tbl,"o3",&y0)) sf_error("No o3= in table");
    if (!sf_histfloat(tbl,"d3",&dy)) sf_error("No d3= in table");

    sf_putint(mig,"n1",nz);
    sf_putint(mig,"n2",nx);

    sf_putfloat(mig,"o1",z0);
    sf_putfloat(mig,"d1",dz);
    sf_putstring(mig,"label1","Depth");
    unit = sf_histstring(inp,"unit2");
    if (NULL != unit) sf_putstring(mig,"unit1",unit);

    sf_putfloat(mig,"o2",x0);
    sf_putfloat(mig,"d2",dx);
    sf_putstring(mig,"label2","Distance");

    if (!sf_getfloat("antialias",&aal)) aal=1.0;
    /* antialiasing */

    nzx = nz*nx;

    /* read traveltime table */
    table = sf_floatalloc2(nzx,ny);
    sf_floatread(table[0],nzx*ny,tbl);
    sf_fileclose(tbl);

    /* read derivative table */
    tablex = sf_floatalloc2(nzx,ny);
    sf_floatread(tablex[0],nzx*ny,der);
    sf_fileclose(der);

    out = sf_floatalloc(nzx);
    trace = sf_floatalloc(nt);

    stable  = sf_floatalloc(nzx);
    stablex = sf_floatalloc(nzx);

    if (NULL == (type = sf_getstring("type"))) type="hermit";
    /* type of interpolation (default Hermit) */

    if (NULL == (what = sf_getstring("what"))) what="expanded";
    /* Hermite basis functions (default expanded) */

    /* initialize interpolation */
    tinterp_init(nzx,dy,what);

    for (i=0; i < nzx; i++) {
	out[i] = 0.;
    }

    for (is=0; is < ns; is++) { /* surface location */
	s = s0 + is*ds;

	/* cubic Hermite spline interpolation */
	ist = (s-y0)/dy;
	if (ist <= 0) {
	    for (ix=0; ix < nzx; ix++) {
		stable[ix]  = table[0][ix];
		stablex[ix] = tablex[0][ix];
	    }
	} else if (ist >= ny-1) {
	    for (ix=0; ix < nzx; ix++) {
		stable[ix]  = table[ny-1][ix];
		stablex[ix] = tablex[ny-1][ix];
	    }
	} else {
	    switch (type[0]) {
		case 'l': /* linear */
		    tinterp_linear(stable, s-ist*dy-y0,table[ist], table[ist+1]);
		    tinterp_linear(stablex,s-ist*dy-y0,tablex[ist],tablex[ist+1]);
		    break;

		case 'h': /* hermit */
		    tinterp_hermite(stable, s-ist*dy-y0,table[ist],table[ist+1],tablex[ist],tablex[ist+1]);
		    dinterp_hermite(stablex,s-ist*dy-y0,table[ist],table[ist+1],tablex[ist],tablex[ist+1]);
		    break;
	    }
	}	

	/* read trace */
	sf_floatread (trace,nt,inp);
	doubint(nt,trace);

	/* Add aperture limitation later */

	for (ix=0; ix < nzx; ix++) { /* image */
	    ti = 2.*stable[ix];
	    tx = 2.*stablex[ix];
	    
	    out[ix] += pick(ti,fabsf(tx*ds*aal),trace,nt,dt,t0);
	}
    }

    sf_floatwrite(out,nzx,mig);        

    exit(0);
}
