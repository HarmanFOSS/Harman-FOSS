/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoViewer.cpp 84 2006-10-14 19:34:29Z dangelo $
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2.1 of
 * the License
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */


#include <libfreepv/QTVRDecoder.h>
#include <libfreepv/utils.h>

using namespace std;
using namespace FPV;

int main(int argc, char *argv[]) 
{
    const char * input = 0;
    std::string prefix;
    if (argc == 2) {
        input = argv[1];
        prefix = "pano";
    } else if (argc == 3) {
        input = argv[1];
        prefix = argv[2];
    } else {
        std::cerr << "QTVR cube extractor." << endl << endl
	              << "This program extracts the cube faces or cylindrical image" << endl
                  << "from a QTVR .mov panorama." << endl << endl
                  << "Usage: qtvr2img movie.mov  [prefix]" << endl
                  << "   movie.mov        The .mov file whose images should be extracted." << endl
                  << "   prefix           Prefix of the cube faces/ cylindrical panorama, " << endl
                  << "                    if not given 'pano' will be used" << endl;
        return 1;
    }

    QTVRDecoder decoder;
    bool ok = false;
    ok = decoder.parseHeaders(argv[1]);
    if (! ok) {
        cerr << "Error parsing QTVR movie: "<< decoder.getErrorDescr() << endl;
        return 1;
    }

    if (decoder.getType() == Parameters::PANO_CYLINDRICAL) {
        DEBUG_TRACE("got a cylindrical pano");
        Image * img = 0;
        if (!decoder.extractCylImage(img)) {
	       cerr << "Decoding of cylindrical panorama failed:" << decoder.getErrorDescr() << endl;
	       return 1;
        } else {
            std::string fn = prefix;
            fn.append(".pnm");
            img->writePPM(fn);
        }
    } else if (decoder.getType() == Parameters::PANO_CUBIC) {
        DEBUG_TRACE("got a cubic pano");
        Image *cubeFaces[6];
        for (int i=0; i < 6 ; i++) {
            cubeFaces[i] = 0;
        };
        std::string decoderError;
        if (decoder.extractCubeImages(cubeFaces))
        {
            for (int i=0; i < 6 ; i++) {
                std::string fn;
                FPV_S2S(fn, prefix << "_" << i << ".pnm");
                cubeFaces[i]->writePPM(fn);
            }
        } else {
            cerr << "Decoding of cubic panorama failed:" << decoder.getErrorDescr() << endl;
            return 1;
        } 
    } else {
        cerr << "Unknown QTVR type" << endl;
        return 1;
    }
    return 0;
}
