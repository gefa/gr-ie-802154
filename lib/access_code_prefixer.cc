/*
 * Copyright (C) 2013 Bastian Bloessl <bloessl@ccs-labs.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ie802_15_4/access_code_prefixer.h>

#include <gnuradio/io_signature.h>
#include <gnuradio/block_detail.h>
#include <string.h>

using namespace gr::ie802_15_4;

class access_code_prefixer_impl : public access_code_prefixer {

	public:

	access_code_prefixer_impl(int pad, int preamble) :
			block("access_code_prefixer",
				gr::io_signature::make(0, 0, 0),
				gr::io_signature::make(0, 0, 0)),
			d_preamble(preamble),
			d_pad(pad) {

	    message_port_register_out(pmt::mp("out"));

	    message_port_register_in(pmt::mp("in"));
	    set_msg_handler(pmt::mp("in"), boost::bind(&access_code_prefixer_impl::make_frame, this, _1));
	    message_port_register_in(pmt::mp("len"));
	    set_msg_handler(pmt::mp("len"), boost::bind(&access_code_prefixer_impl::set_len, this, _1)); 
	    buf[0] = 0 & 0xFF; //pad & 0xFF;
	    //std::cout << "FRAAAAAAME"<<std::endl;

	    for(int i = 4; i > 0; i--) {
		buf[i] = d_preamble & 0xFF;
		d_preamble >>= 8;
	    }
	}

	~access_code_prefixer_impl() {

	}

	void set_len(pmt::pmt_t len) {
		int psdu_length2 = pmt::blob_length(pmt::cdr(len));
		std::cout << "LEN bytes" << psdu_length2 << std::endl;
		const char *length = static_cast<const char*>(pmt::blob_data(pmt::cdr(len)));
		if (psdu_length2 == 1) { // accept single byte message only
			d_pad = (length[0]-48);
		}
		std::cout << "LEN value" << d_pad << std::endl;
	}

	void make_frame (pmt::pmt_t msg) {

		if(pmt::is_eof_object(msg)) {
			message_port_pub(pmt::mp("out"), pmt::PMT_EOF);
			detail().get()->set_done(true);
			return;
		}

		assert(pmt::is_pair(msg));
		pmt::pmt_t blob = pmt::cdr(msg);

		size_t data_len = pmt::blob_length(blob);
		assert(data_len);
		assert(data_len < 256 - 5);
		// override our fake frame length, single byte only
		buf[5] = d_pad & 0xFF;//data_len;

		std::memcpy(buf + 6, pmt::blob_data(blob), data_len);
		pmt::pmt_t packet = pmt::make_blob(buf, 6 + 10);//data_len + 6);
		//std::cout << "FRAME LEN "<< d_pad<<std::endl;
		message_port_pub(pmt::mp("out"), pmt::cons(pmt::PMT_NIL, packet));
	}

	private:
		char		buf[256];
		unsigned int	d_preamble;
		unsigned int    d_pad;

};

access_code_prefixer::sptr
access_code_prefixer::make(int pad, int preamble) {
	return gnuradio::get_initial_sptr(new access_code_prefixer_impl(pad,preamble));
}


