#include "HTTPDownloader.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>


namespace detail {

	template<class = void>
	void
	load_root_certificates(ssl::context& ctx, boost::system::error_code& ec)
	{
		std::string const cert =
			/*  This is the DigiCert root certificate.
				
				CN = DigiCert High Assurance EV Root CA
				OU = www.digicert.com
				O = DigiCert Inc
				C = US

				Valid to: Sunday, ?November ?9, ?2031 5:00:00 PM
									
				Thumbprint(sha1):
				5f b7 ee 06 33 e2 59 db ad 0c 4c 9a e6 d3 8f 1a 61 c7 dc 25
			*/
			"-----BEGIN CERTIFICATE-----\n"
			"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
			"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
			"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
			"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
			"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
			"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
			"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
			"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
			"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
			"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
			"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
			"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
			"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
			"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
			"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
			"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
			"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
			"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
			"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
			"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
			"+OkuE6N36B9K\n"
			"-----END CERTIFICATE-----\n"
			/*  This is the GeoTrust root certificate.
				
				CN = GeoTrust Global CA
				O = GeoTrust Inc.
				C = US
				Valid to: Friday, ‎May ‎20, ‎2022 9:00:00 PM
									
				Thumbprint(sha1):
				‎de 28 f4 a4 ff e5 b9 2f a3 c5 03 d1 a3 49 a7 f9 96 2a 82 12
			*/
			;

		ctx.add_certificate_authority(
			boost::asio::buffer(cert.data(), cert.size()), ec);
		if(ec)
			return;
	}

} 

inline void load_root_certificates(ssl::context& ctx, boost::system::error_code& ec)
{
    detail::load_root_certificates(ctx, ec);
}

inline void load_root_certificates(ssl::context& ctx)
{
    boost::system::error_code ec;
    detail::load_root_certificates(ctx, ec);
    if(ec)
        throw boost::system::system_error{ec};
}

std::string HTTPDownloader::LoadHTTP(const std::string& host, const std::string& port, const std::string& target)
{
	int version = 10;
	boost::asio::io_context ioc;

	tcp::resolver resolver{ioc};
	tcp::socket socket{ioc};

	auto const results = resolver.resolve(host, port);

	boost::asio::connect(socket, results.begin(), results.end());

	http::request<http::string_body> req{http::verb::get, target, version};
	req.set(http::field::host, host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	http::write(socket, req);

	boost::beast::flat_buffer buffer;

	http::response<http::dynamic_body> res;

	http::read(socket, buffer, res);

	std::string strres = boost::beast::buffers_to_string(res.body().data());
	
	boost::system::error_code ec;
	socket.shutdown(tcp::socket::shutdown_both, ec);

	if(ec && ec != boost::system::errc::not_connected)
		throw boost::system::system_error{ec};

	return strres;
}

std::string HTTPDownloader::LoadHTTPS(const std::string& host, const std::string& port, const std::string& target)
{
	int version = 10;

	boost::asio::io_context ioc;

	ssl::context ctx{ssl::context::sslv23_client};

	load_root_certificates(ctx);

	ctx.set_verify_mode(ssl::verify_peer);

	tcp::resolver resolver{ioc};
	ssl::stream<tcp::socket> stream{ioc, ctx};

	if(! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
	{
		boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
		throw boost::system::system_error{ec};
	}

	auto const results = resolver.resolve(host, port);

	boost::asio::connect(stream.next_layer(), results.begin(), results.end());

	stream.handshake(ssl::stream_base::client);

	http::request<http::string_body> req{http::verb::get, target, version};
	req.set(http::field::host, host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	http::write(stream, req);

	boost::beast::flat_buffer buffer;

	http::response<http::dynamic_body> res;

	http::read(stream, buffer, res);

	std::string strres = boost::beast::buffers_to_string(res.body().data());

	boost::system::error_code ec;
	stream.shutdown(ec);
	if(ec == boost::asio::error::eof)
	{
		// Rationale:
		// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
		ec.assign(0, ec.category());
	}
	if(ec)
		throw boost::system::system_error{ec};

	return strres;
}
