/***************************************************************************
 *
 * Author: "Sjors H.W. Scheres"
 * MRC Laboratory of Molecular Biology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This complete copyright notice must be included in any revised version of the
 * source code. Additional authorship citations may be added, but existing
 * author citations must be preserved.
 ***************************************************************************/
#include "src/reconstructor.h"

void Reconstructor::read(int argc, char **argv)
{
	parser.setCommandLine(argc, argv);

	int general_section = parser.addSection("General options");
	fn_sel = parser.getOption("--i", "Input STAR file with the projection images and their orientations", "");
	fn_out = parser.getOption("--o", "Name for output reconstruction","relion.mrc");
	fn_sym = parser.getOption("--sym", "Symmetry group", "c1");
	maxres = textToFloat(parser.getOption("--maxres", "Maximum resolution (in Angstrom) to consider in Fourier space (default Nyquist)", "-1"));
	padding_factor = textToFloat(parser.getOption("--pad", "Padding factor", "2"));
	image_path = parser.getOption("--img", "Optional: image path prefix", "");
	subset = textToInteger(parser.getOption("--subset", "Subset of images to consider (1: only reconstruct half1; 2: only half2; other: reconstruct all)", "-1"));

	int ctf_section = parser.addSection("CTF options");
	do_ctf = parser.checkOption("--ctf", "Apply CTF correction");
	intact_ctf_first_peak = parser.checkOption("--ctf_intact_first_peak", "Leave CTFs intact until first peak");
	ctf_phase_flipped = parser.checkOption("--ctf_phase_flipped", "Images have been phase flipped");
	only_flip_phases = parser.checkOption("--only_flip_phases", "Do not correct CTF-amplitudes, only flip phases");
	ctf_premultiplied = parser.checkOption("--ctf_multiplied", "Have the data been premultiplied with their CTF?");

	int ewald_section = parser.addSection("Ewald-sphere correction options");
	do_ewald = parser.checkOption("--ewald", "Correct for Ewald-sphere curvature (developmental)");
	mask_diameter  = textToFloat(parser.getOption("--mask_diameter", "Diameter (in A) of mask for Ewald-sphere curvature correction", "-1."));
	width_mask_edge = textToInteger(parser.getOption("--width_mask_edge", "Width (in pixels) of the soft edge on the mask", "3"));
	is_reverse = parser.checkOption("--reverse_curvature", "Try curvature the other way around");
	newbox = textToInteger(parser.getOption("--newbox", "Box size of reconstruction after Ewald sphere correction", "-1"));
	nr_sectors = textToInteger(parser.getOption("--sectors", "Number of sectors for Ewald sphere correction", "2"));
	skip_mask = parser.checkOption("--skip_mask", "Do not apply real space mask during Ewald sphere correction");
	skip_weighting = parser.checkOption("--skip_weighting", "Do not apply weighting during Ewald sphere correction");

	if (verb > 0 && do_ewald && mask_diameter < 0 && !(skip_mask && skip_weighting))
		REPORT_ERROR("To apply Ewald sphere correction (--ewald), you have to specify the mask diameter(--mask_diameter).");

	int helical_section = parser.addSection("Helical options");
	nr_helical_asu = textToInteger(parser.getOption("--nr_helical_asu", "Number of helical asymmetrical units", "1"));
	helical_rise = textToFloat(parser.getOption("--helical_rise", "Helical rise (in Angstroms)", "0."));
	helical_twist = textToFloat(parser.getOption("--helical_twist", "Helical twist (in degrees, + for right-handedness)", "0."));

	int expert_section = parser.addSection("Expert options");
	fn_sub = parser.getOption("--subtract","Subtract projections of this map from the images used for reconstruction", "");
	if (parser.checkOption("--NN", "Use nearest-neighbour instead of linear interpolation before gridding correction"))
		interpolator = NEAREST_NEIGHBOUR;
	else
		interpolator = TRILINEAR;
	blob_radius = textToFloat(parser.getOption("--blob_r", "Radius of blob for gridding interpolation", "1.9"));
	blob_order = textToInteger(parser.getOption("--blob_m", "Order of blob for gridding interpolation", "0"));
	blob_alpha = textToFloat(parser.getOption("--blob_a", "Alpha-value of blob for gridding interpolation", "15"));
	iter = textToInteger(parser.getOption("--iter", "Number of gridding-correction iterations", "10"));
	ref_dim = textToInteger(parser.getOption("--refdim", "Dimension of the reconstruction (2D or 3D)", "3"));
	angular_error = textToFloat(parser.getOption("--angular_error", "Apply random deviations with this standard deviation (in degrees) to each of the 3 Euler angles", "0."));
	shift_error = textToFloat(parser.getOption("--shift_error", "Apply random deviations with this standard deviation (in Angstrom) to each of the 2 translations", "0."));
	do_fom_weighting = parser.checkOption("--fom_weighting", "Weight particles according to their figure-of-merit (_rlnParticleFigureOfMerit)");
	fn_fsc = parser.getOption("--fsc", "FSC-curve for regularized reconstruction", "");
	do_3d_rot = parser.checkOption("--3d_rot", "Perform 3D rotations instead of backprojections from 2D images");
	ctf_dim  = textToInteger(parser.getOption("--reconstruct_ctf", "Perform a 3D reconstruction from 2D CTF-images, with the given size in pixels", "-1"));
	do_reconstruct_ctf2 = parser.checkOption("--ctf2", "Reconstruct CTF^2 and then take the sqrt of that");
	skip_gridding = parser.checkOption("--skip_gridding", "Skip gridding part of the reconstruction");
	fn_debug = parser.getOption("--debug", "Rootname for debug reconstruction files", "");
	debug_ori_size =  textToInteger(parser.getOption("--debug_ori_size", "Rootname for debug reconstruction files", "1"));
	debug_size =  textToInteger(parser.getOption("--debug_size", "Rootname for debug reconstruction files", "1"));
	read_weights = parser.checkOption("--read_weights", "Developmental: read freq. weight files");
	do_debug = parser.checkOption("--write_debug_output", "Write out arrays with data and weight terms prior to reconstruct");
	verb = textToInteger(parser.getOption("--verb", "Verbosity", "1"));

	// Hidden
	r_min_nn = textToInteger(getParameter(argc, argv, "--r_min_nn", "10"));

	// Check for errors in the command-line option
	if (parser.checkForErrors())
		REPORT_ERROR("Errors encountered on the command line (see above), exiting...");
}

void Reconstructor::usage()
{
	parser.writeUsage(std::cout);
}

void Reconstructor::initialise()
{
	do_reconstruct_ctf = (ctf_dim > 0);
	if (do_reconstruct_ctf)
		do_ctf = false;

	// Read MetaData file, which should have the image names and their angles!
	if (fn_debug == "")
	{
		ObservationModel::loadSafely(fn_sel, obsModel, DF);
	}

	if (verb > 0 && (subset == 1 || subset == 2) && !DF.containsLabel(EMDL_PARTICLE_RANDOM_SUBSET))
	{
		REPORT_ERROR("The rlnRandomSubset column is missing in the input STAR file.");
	}

	randomize_random_generator();

	//if (cl_beamtilt || do_ewald)
		do_ctf = true;

	// Is this 2D or 3D data?
	data_dim = (do_3d_rot) ? 3 : 2;

	// Get dimension of the images
	if (do_reconstruct_ctf)
	{
		mysize = ctf_dim;
	}
	else
	{
		(DF).firstObject();
		DF.getValue(EMDL_IMAGE_NAME, fn_img);

		if (image_path != "")
		{
			fn_img = image_path + "/" + fn_img.substr(fn_img.find_last_of("/")+1);
		}

		Image<RFLOAT> img0;
		img0.read(fn_img, false);
		mysize=(int)XSIZE(img0());
		// When doing Ewald-curvature correction: allow reconstructing smaller box than the input images (which should have large boxes!!)
		if (do_ewald && newbox > 0)
			mysize = newbox;
	}

	if (!obsModel.allPixelSizesIdentical())
	{
		REPORT_ERROR("Reconstructor does not support varying pixel sizes yet.");
	}

	angpix = obsModel.getPixelSize(0);

	if (maxres < 0.)
		r_max = -1;
	else
		r_max = CEIL(mysize * angpix / maxres);

}

void Reconstructor::run()
{
	if (fn_debug != "")
	{
		readDebugArrays();
	}
	else
	{
		initialise();
		backproject();
	}

	reconstruct();
}

void Reconstructor::readDebugArrays()
{
	if (verb > 0)
		std::cout << " + Reading in the debug arrays ... " << std::endl;
	data_dim = (do_3d_rot) ? 3 : 2;

	backprojector = BackProjector(debug_ori_size, 3, fn_sym, interpolator, padding_factor, r_min_nn, blob_order, blob_radius, blob_alpha, data_dim, skip_gridding);

	backprojector.initialiseDataAndWeight(debug_size);
	if (verb > 0)
	{
		std::cout << " Size of data array: " ;
		backprojector.data.printShape();
		std::cout << " Size of weight array: " ;
		backprojector.weight.printShape();
	}
	Image<RFLOAT> It;
	It.read(fn_debug+"_data_real.mrc");
	It().setXmippOrigin();
	It().xinit=0;

	if (verb > 0)
	{
		std::cout << " Size of reconstruction: " ;
		It().printShape();
	}
	FOR_ALL_ELEMENTS_IN_ARRAY3D(It())
	{
		A3D_ELEM(backprojector.data, k, i, j).real = A3D_ELEM(It(), k, i, j);
	}
	It.read(fn_debug+"_data_imag.mrc");
	It().setXmippOrigin();
	It().xinit=0;
	FOR_ALL_ELEMENTS_IN_ARRAY3D(It())
	{
		A3D_ELEM(backprojector.data, k, i, j).imag = A3D_ELEM(It(), k, i, j);
	}
	It.read(fn_debug+"_weight.mrc");
	It().setXmippOrigin();
	It().xinit=0;
	FOR_ALL_ELEMENTS_IN_ARRAY3D(It())
	{
		A3D_ELEM(backprojector.weight, k, i, j) = A3D_ELEM(It(), k, i, j);
	}
	mysize = debug_ori_size;
}

void Reconstructor::backproject(int rank, int size)
{
	if (fn_sub != "")
	{
		projector = Projector(mysize, interpolator, padding_factor, r_min_nn);
		Image<RFLOAT> sub;
		sub.read(fn_sub);
		MultidimArray<RFLOAT> dummy;
		projector.computeFourierTransformMap(sub(), dummy, 2 * r_max);
	}

	backprojector = BackProjector(mysize, ref_dim, fn_sym, interpolator,
					padding_factor, r_min_nn, blob_order,
					blob_radius, blob_alpha, data_dim, skip_gridding);
	backprojector.initZeros(2 * r_max);

	long int nr_parts = DF.numberOfObjects();
	long int barstep = XMIPP_MAX(1, nr_parts/(size*120));
	if (verb > 0)
	{
		std::cout << " + Back-projecting all images ..." << std::endl;
		time_config();
		init_progress_bar(nr_parts);
	}

	for (long int ipart = 0; ipart < nr_parts; ipart++)
	{
		if (ipart % size == rank)
			backprojectOneParticle(ipart);

		if (ipart % barstep == 0 && verb > 0)
			progress_bar(ipart);
	}

	if (verb > 0)
		progress_bar(nr_parts);
}

void Reconstructor::backprojectOneParticle(long int p)
{
	RFLOAT rot, tilt, psi, fom, r_ewald_sphere;
	Matrix2D<RFLOAT> A3D;
	MultidimArray<RFLOAT> Fctf;
	Matrix1D<RFLOAT> trans(2);
	FourierTransformer transformer;

	int randSubset = 0;
	DF.getValue(EMDL_PARTICLE_RANDOM_SUBSET, randSubset, p);

	if (subset >= 1 && subset <= 2 && randSubset != subset)
		return;

	// Rotations
	if (ref_dim == 2)
	{
		rot = tilt = 0.;
	}
	else
	{
		DF.getValue(EMDL_ORIENT_ROT, rot, p);
		DF.getValue(EMDL_ORIENT_TILT, tilt, p);
	}

	psi = 0.;
	DF.getValue(EMDL_ORIENT_PSI, psi, p);

	if (angular_error > 0.)
	{
		rot += rnd_gaus(0., angular_error);
		tilt += rnd_gaus(0., angular_error);
		psi += rnd_gaus(0., angular_error);
		//std::cout << rnd_gaus(0., angular_error) << std::endl;
	}

	Euler_angles2matrix(rot, tilt, psi, A3D);

	// Translations (either through phase-shifts or in real space
	trans.initZeros();
	DF.getValue( EMDL_ORIENT_ORIGIN_X_ANGSTROM, XX(trans), p);
	DF.getValue( EMDL_ORIENT_ORIGIN_Y_ANGSTROM, YY(trans), p);

	if (shift_error > 0.)
	{
		XX(trans) += rnd_gaus(0., shift_error);
		YY(trans) += rnd_gaus(0., shift_error);
	}

	if (do_3d_rot)
	{
		trans.resize(3);
		DF.getValue( EMDL_ORIENT_ORIGIN_Z_ANGSTROM, ZZ(trans), p);

		if (shift_error > 0.)
		{
			ZZ(trans) += rnd_gaus(0., shift_error);
		}
	}

	// As of v3.1, shifts are in Angstroms in the STAR files, convert back to pixels here
	trans/= angpix;

	if (do_fom_weighting)
	{
		DF.getValue( EMDL_PARTICLE_FOM, fom, p);
	}

	// Use either selfTranslate OR shiftImageInFourierTransform!!
	//selfTranslate(img(), trans, WRAP);

	MultidimArray<Complex> Fsub, F2D, F2DP, F2DQ;
	FileName fn_img;
	Image<RFLOAT> img;
    if (do_reconstruct_ctf)
    {
        mysize = ctf_dim;
        img().initZeros(ctf_dim, ctf_dim);
        img().setXmippOrigin();
    }
    else
    {
		DF.getValue( EMDL_IMAGE_NAME, fn_img, p);
		img.read(fn_img);
		img().setXmippOrigin();
		CenterFFT(img(), true);
		transformer.FourierTransform(img(), F2D);
    }

	if (do_3d_rot)
	{
		if (ABS(XX(trans)) > 0. || ABS(YY(trans)) > 0. || ABS(ZZ(trans)) > 0. )
		{
			shiftImageInFourierTransform(F2D, F2D,
				XSIZE(img()), XX(trans), YY(trans), ZZ(trans));
		}
	}
	else
	{
		if (ABS(XX(trans)) > 0. || ABS(YY(trans)) > 0.)
		{
			shiftImageInFourierTransform(F2D, F2D,
				 XSIZE(img()), XX(trans), YY(trans));
		}
	}

	Fctf.resize(F2D);
	Fctf.initConstant(1.);

	// Apply CTF if necessary
	if (do_ctf || do_reconstruct_ctf)
	{

		// Also allow 3D CTF correction here
		if (data_dim == 3)
		{
			Image<RFLOAT> Ictf;
			FileName fn_ctf;
			if (!DF.getValue(EMDL_CTF_IMAGE, fn_ctf, p))
				REPORT_ERROR("ERROR: cannot find rlnCtfImage for 3D CTF correction!");
			Ictf.read(fn_ctf);

			// If there is a redundant half, get rid of it
			if (XSIZE(Ictf()) == YSIZE(Ictf()))
			{
				Ictf().setXmippOrigin();
				FOR_ALL_ELEMENTS_IN_FFTW_TRANSFORM(Fctf)
				{
					// Use negative kp,ip and jp indices, because the origin in the ctf_img lies half a pixel to the right of the actual center....
					DIRECT_A3D_ELEM(Fctf, k, i, j) = A3D_ELEM(Ictf(), -kp, -ip, -jp);
				}
			}
			// otherwise, just window the CTF to the current resolution
			else if (XSIZE(Ictf()) == YSIZE(Ictf()) / 2 + 1)
			{
				windowFourierTransform(Ictf(), Fctf, YSIZE(Fctf));
			}
			// if dimensions are neither cubical nor FFTW, stop
			else
			{
				REPORT_ERROR("3D CTF volume must be either cubical or adhere to FFTW format!");
			}
		}
		else
		{
			CTF ctf;
			ctf.readByGroup(DF, &obsModel, p);

			ctf.getFftwImage(Fctf, mysize, mysize, angpix,
				 ctf_phase_flipped, only_flip_phases,
				 intact_ctf_first_peak, true);

			obsModel.demodulatePhase(DF, p, F2D);

			// Ewald-sphere curvature correction
			if (do_ewald)
			{
				applyCTFPandCTFQ(F2D, ctf, transformer, F2DP, F2DQ, skip_mask);

				if (!skip_weighting)
				{
					// Also calculate W, store again in Fctf
					ctf.applyWeightEwaldSphereCurvature_noAniso(Fctf, mysize, mysize, angpix, mask_diameter);
				}

				// Also calculate the radius of the Ewald sphere (in pixels)
				r_ewald_sphere = mysize * angpix / ctf.lambda;
			}
		}
	}

	// Subtract reference projection
	if (fn_sub != "")
	{
		Fsub.resize(F2D);
		projector.get2DFourierTransform(Fsub, A3D, IS_NOT_INV);

		// Apply CTF if necessary
		if (do_ctf)
		{
			FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Fsub)
			{
				DIRECT_MULTIDIM_ELEM(Fsub, n) *= DIRECT_MULTIDIM_ELEM(Fctf, n);
			}
		}

		FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Fsub)
		{
			DIRECT_MULTIDIM_ELEM(F2D, n) -= DIRECT_MULTIDIM_ELEM(Fsub, n);
		}
		// Back-project difference image
		backprojector.set2DFourierTransform(F2D, A3D, IS_NOT_INV);
	}
	else
	{
		if (do_reconstruct_ctf)
		{
			FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(F2D)
			{
				DIRECT_MULTIDIM_ELEM(F2D, n)  = DIRECT_MULTIDIM_ELEM(Fctf, n);
				if (do_reconstruct_ctf2)
					DIRECT_MULTIDIM_ELEM(F2D, n) *= DIRECT_MULTIDIM_ELEM(Fctf, n);
				DIRECT_MULTIDIM_ELEM(Fctf, n) = 1.;
			}
		}
		else if (do_ewald)
		{
			FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(F2D)
			{
				DIRECT_MULTIDIM_ELEM(Fctf, n) *= DIRECT_MULTIDIM_ELEM(Fctf, n);
			}
		}
		// "Normal" reconstruction, multiply X by CTF, and W by CTF^2
		else if (do_ctf)
		{
			if (!ctf_premultiplied)
			{
				FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(F2D)
				{
					DIRECT_MULTIDIM_ELEM(F2D, n)  *= DIRECT_MULTIDIM_ELEM(Fctf, n);
				}
			}
			FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Fctf)
			{
				DIRECT_MULTIDIM_ELEM(Fctf, n) *= DIRECT_MULTIDIM_ELEM(Fctf, n);
			}
		}

		// Do the following after squaring the CTFs!
		if (do_fom_weighting)
		{
			FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(F2D)
			{
				DIRECT_MULTIDIM_ELEM(F2D, n)  *= fom;
				DIRECT_MULTIDIM_ELEM(Fctf, n) *= fom;
			}
		}

		if (read_weights)
		{
			std::string name, fullName;

			DF.getValue(EMDL_IMAGE_NAME, fullName, 0);
			name = fullName.substr(fullName.find("@")+1);

			if (image_path != "")
			{
				name = image_path + "/" + name.substr(name.find_last_of("/")+1);
			}

			std::string wghName = name;
			wghName = wghName.substr(0, wghName.find_last_of('.')) + "_weight.mrc";

			Image<RFLOAT> wgh;
			wgh.read(wghName);

			if (   Fctf.ndim != wgh().ndim
			    || Fctf.zdim != wgh().zdim
			    || Fctf.ydim != wgh().ydim
			    || Fctf.xdim != wgh().xdim)
			{
				REPORT_ERROR(wghName + " and " + name + " are of unequal size.\n");
			}

			for (long int n = 0; n < Fctf.ndim; n++)
			for (long int z = 0; z < Fctf.zdim; z++)
			for (long int y = 0; y < Fctf.ydim; y++)
			for (long int x = 0; x < Fctf.xdim; x++)
			{
				DIRECT_NZYX_ELEM(Fctf, n, z, y, x)
						*= DIRECT_NZYX_ELEM(wgh(), n, z, y, x);
			}
		}

		DIRECT_A2D_ELEM(F2D, 0, 0) = 0.0;

		if (do_ewald)
		{
			backprojector.set2DFourierTransform(F2DP, A3D, IS_NOT_INV, &Fctf, r_ewald_sphere, true);
			backprojector.set2DFourierTransform(F2DQ, A3D, IS_NOT_INV, &Fctf, r_ewald_sphere, false);
		}
		else
		{
			backprojector.set2DFourierTransform(F2D, A3D, IS_NOT_INV, &Fctf);
		}
	}


}

void Reconstructor::reconstruct()
{
	bool do_map = false;
	bool do_use_fsc = false;
	MultidimArray<RFLOAT> fsc, dummy;
	Image<RFLOAT> vol;
	fsc.resize(mysize/2+1);

	if (fn_fsc != "")
	{
		do_map = true;
		do_use_fsc =true;
		MetaDataTable MDfsc;
		MDfsc.read(fn_fsc);
		FOR_ALL_OBJECTS_IN_METADATA_TABLE(MDfsc)
		{
			int idx;
			RFLOAT val;
			MDfsc.getValue(EMDL_SPECTRAL_IDX, idx);
			MDfsc.getValue(EMDL_MLMODEL_FSC_HALVES_REF, val);
			fsc(idx) =	val;
		}
	}

	if (verb > 0)
		std::cout << " + Starting the reconstruction ..." << std::endl;
	backprojector.symmetrise(nr_helical_asu, helical_twist, helical_rise/angpix);


	if (do_debug)
	{
		Image<RFLOAT> It;
		FileName fn_tmp = fn_out.withoutExtension();
		It().resize(backprojector.data);
		FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(It())
		{
			DIRECT_MULTIDIM_ELEM(It(), n) = (DIRECT_MULTIDIM_ELEM(backprojector.data, n)).real;
		}
		It.write(fn_tmp+"_data_real.mrc");
		FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(It())
		{
			DIRECT_MULTIDIM_ELEM(It(), n) = (DIRECT_MULTIDIM_ELEM(backprojector.data, n)).imag;
		}
		It.write(fn_tmp+"_data_imag.mrc");
		It()=backprojector.weight;
		It.write(fn_tmp+"_weight.mrc");
	}

	backprojector.reconstruct(vol(), iter, do_map, 1., dummy, dummy, dummy, dummy,
							  fsc, 1., do_use_fsc, true, 1, -1, false);


	if (do_reconstruct_ctf)
	{
		MultidimArray<Complex> F2D;
		FourierTransformer transformer;

		F2D.clear();
		transformer.FourierTransform(vol(), F2D);

		// CenterOriginFFT: Set the center of the FFT in the FFTW origin
		Matrix1D<RFLOAT> shift(3);
		XX(shift)=-(RFLOAT)(int)(ctf_dim / 2);
		YY(shift)=-(RFLOAT)(int)(ctf_dim / 2);
		ZZ(shift)=-(RFLOAT)(int)(ctf_dim / 2);
		shiftImageInFourierTransform(F2D, F2D, (RFLOAT)ctf_dim, XX(shift), YY(shift), ZZ(shift));
		vol().setXmippOrigin();
		vol().initZeros();
		FOR_ALL_ELEMENTS_IN_FFTW_TRANSFORM(F2D)
		{
			// Take care of kp==dim/2, as XmippOrigin lies just right off center of image...
			if ( kp > FINISHINGZ(vol()) || ip > FINISHINGY(vol()) || jp > FINISHINGX(vol()))
				continue;
			A3D_ELEM(vol(), kp, ip, jp)    = FFTW_ELEM(F2D, kp, ip, jp).real;
			A3D_ELEM(vol(), -kp, -ip, -jp) = FFTW_ELEM(F2D, kp, ip, jp).real;
		}
		vol() *= (RFLOAT)ctf_dim;

		// Take sqrt(CTF^2)
		if (do_reconstruct_ctf2)
		{
			FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(vol())
			{
				if (DIRECT_MULTIDIM_ELEM(vol(), n) > 0.)
					DIRECT_MULTIDIM_ELEM(vol(), n) = sqrt(DIRECT_MULTIDIM_ELEM(vol(), n));
				else
					DIRECT_MULTIDIM_ELEM(vol(), n) = 0.;
			}
		}
	}

	vol.setSamplingRateInHeader(angpix);
	vol.write(fn_out);
	if (verb > 0)
		std::cout << " + Done! Written output map in: "<<fn_out<<std::endl;


}

void Reconstructor::applyCTFPandCTFQ(MultidimArray<Complex> &Fin, CTF &ctf, FourierTransformer &transformer,
		MultidimArray<Complex> &outP, MultidimArray<Complex> &outQ, bool skip_mask)
{
	//FourierTransformer transformer;
	outP.resize(Fin);
	outQ.resize(Fin);
	float angle_step = 180./nr_sectors;
	for (float angle = 0.; angle < 180.;  angle +=angle_step)
	{
		MultidimArray<Complex> CTFP(Fin), Fapp(Fin);
		MultidimArray<RFLOAT> Iapp(YSIZE(Fin), YSIZE(Fin));
		// Two passes: one for CTFP, one for CTFQ
		for (int ipass = 0; ipass < 2; ipass++)
		{
			bool is_my_positive = (ipass == 1) ? is_reverse : !is_reverse;

			// Get CTFP and multiply the Fapp with it
			ctf.getCTFPImage(CTFP, YSIZE(Fin), YSIZE(Fin), angpix, is_my_positive, angle);

			Fapp = Fin * CTFP; // element-wise complex multiplication!

			if (!skip_mask)
			{
				// inverse transform and mask out the particle....
				transformer.inverseFourierTransform(Fapp, Iapp);
				CenterFFT(Iapp, false);

				softMaskOutsideMap(Iapp, ROUND(mask_diameter/(angpix*2.)), (RFLOAT)width_mask_edge);

				// Re-box to a smaller size if necessary....
				if (newbox > 0 && newbox < YSIZE(Fin))
				{
					Iapp.setXmippOrigin();
					Iapp.window(FIRST_XMIPP_INDEX(newbox), FIRST_XMIPP_INDEX(newbox),
					            LAST_XMIPP_INDEX(newbox),  LAST_XMIPP_INDEX(newbox));

				}
				// Back into Fourier-space
				CenterFFT(Iapp, true);
				transformer.FourierTransform(Iapp, Fapp, false); // false means: leave Fapp in the transformer
			}

			// First time round: resize the output arrays
			if (ipass == 0 && fabs(angle) < XMIPP_EQUAL_ACCURACY)
			{
				outP.resize(Fapp);
				outQ.resize(Fapp);
			}

			// Now set back the right parts into outP (first pass) or outQ (second pass)
			float anglemin = angle + 90. - (0.5*angle_step);
			float anglemax = angle + 90. + (0.5*angle_step);

			// angles larger than 180
			bool is_reverse = false;
			if (anglemin >= 180.)
			{
				anglemin -= 180.;
				anglemax -= 180.;
				is_reverse = true;
			}
			MultidimArray<Complex> *myCTFPorQ, *myCTFPorQb;
			if (is_reverse)
			{
				myCTFPorQ  = (ipass == 0) ? &outQ : &outP;
				myCTFPorQb = (ipass == 0) ? &outP : &outQ;
			}
			else
			{
				myCTFPorQ  = (ipass == 0) ? &outP : &outQ;
				myCTFPorQb = (ipass == 0) ? &outQ : &outP;
			}

			// Deal with sectors with the Y-axis in the middle of the sector...
			bool do_wrap_max = false;
			if (anglemin < 180. && anglemax > 180.)
			{
				anglemax -= 180.;
				do_wrap_max = true;
			}

			// use radians instead of degrees
			anglemin = DEG2RAD(anglemin);
			anglemax = DEG2RAD(anglemax);
			FOR_ALL_ELEMENTS_IN_FFTW_TRANSFORM2D(CTFP)
			{
				RFLOAT x = (RFLOAT)jp;
				RFLOAT y = (RFLOAT)ip;
				RFLOAT myangle = (x*x+y*y > 0) ? acos(y/sqrt(x*x+y*y)) : 0; // dot-product with Y-axis: (0,1)
				// Only take the relevant sector now...
				if (do_wrap_max)
				{
					if (myangle >= anglemin)
						DIRECT_A2D_ELEM(*myCTFPorQ, i, j) = DIRECT_A2D_ELEM(Fapp, i, j);
					else if (myangle < anglemax)
						DIRECT_A2D_ELEM(*myCTFPorQb, i, j) = DIRECT_A2D_ELEM(Fapp, i, j);
				}
				else
				{
					if (myangle >= anglemin && myangle < anglemax)
						DIRECT_A2D_ELEM(*myCTFPorQ, i, j) = DIRECT_A2D_ELEM(Fapp, i, j);
				}
			}
		}
	}
}


