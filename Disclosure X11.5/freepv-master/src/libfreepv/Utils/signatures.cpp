/* -*- c-basic-offset: 4 -*- */
/*
 * This file is part of the freepv panoramic viewer.
 *
 *  Author: Leon Moctezuma <densedev_at_gmail_dot_com>
 *
 *  $Id$
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

#include "signatures.h"
#include <iostream>
#include <cstdio>

namespace FPV
{
  namespace Utils
  {

    //JPEG Signature
    char jpg_size = 1;
    unsigned short jpg_sig[] = {0xD8FF};
    //PNG Signature
    char png_size = 4;
    unsigned short png_sig[] = {0x5089, 0x474E, 0x0A0D, 0x0A1A};
    //XML Signature
    char xml_size = 3;
    unsigned short xml_sig[] = {0x3F3C, 0x6D78, 0x206C};
    //DCR Signature
    char dcr_size = 2;
    unsigned short dcr_sig[] = {0x4658, 0x5249};
    //MOV (MOOV atom) Signature
    char mov_moov_size = 4;
    unsigned short mov_moov_sig[] = {0x0000, 0x0000, 0x6F6D, 0x766F};
    //MOV (FTYP atom) Siganture
    char mov_ftyp_size = 4;
    unsigned short mov_ftyp_sig[] = {0x0000, 0x0000, 0x7466, 0x7079};

    //Signatures to check
    char sig_types_num = 6;
    unsigned short *signature[] = {jpg_sig, png_sig, xml_sig, dcr_sig, mov_moov_sig, mov_ftyp_sig};
    char sig_sizes[] = {jpg_size, png_size, xml_size, dcr_size, mov_moov_size, mov_ftyp_size};

    const char *CheckMagicBytes(const char *filename)
    {
      FILE *p_file;
      p_file = fopen(filename, "rb");
      unsigned short number;
      unsigned short equal = 0;
      char i, type;

      if (p_file == NULL)
      {
        return "Unknown";
      }

      type = 0;
      do
      {
        rewind(p_file);
        i = 0;
        do
        {
          fread(&number, 2, 1, p_file);

          //In case of MOV skip the first 4 bytes.
          //by asigning the first 4 read bytes
          //to the MOV signature
          if (type > 2 && i < 2)
            mov_ftyp_sig[i] = mov_moov_sig[i] = number;

          //The comparation is made by using a bit XOR operator
          //we look for the double implication <->, so the result
          //of the XOR operation is negated.
          equal = !(signature[type][i++] ^ number);

          //std::cerr<<"Lecture:"<<std::hex<<signature[type][i]<<std::endl;
          //std::cerr<<"Compared with:"<<std::hex<<number<<std::endl<<std::endl;
          //std::cerr<<"Equal: "<<std::hex<<equal<<std::endl<<std::endl;*/

        } while (equal && i < sig_sizes[type]);

        if (equal)
        {
          //The signature was found,
          //There is no need to keep looking for...
          break;
        }
        //Check next probable signature.
        type++;

      } while (type < sig_types_num);

      fclose(p_file);

      if (type == 0)
        return ("JPG");
      if (type == 1)
        return ("PNG");
      ;
      if (type == 2)
        return ("XML");
      if (type == 3)
        return ("DCR");
      if (type == 4 || type == 5)
        return ("QTVR");

      //In other case return Unknown
      return ("Unknown");
    }

  } // namespace Utils
} // namespace FPV
