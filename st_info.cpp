#include "st_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <thread>
#include <windows.h>

static char error_buffer[CURL_ERROR_SIZE];
st_info::st_info()
	:_price(0)
	, _need_exit(false)
	, _current_record_pos(0)
	, _total_record(0)
	, _need_judge(true)
{
}

st_info::~st_info()
{

}

void st_info::set_stock_id(py_stock* sts, int num,float resm)
{
	for (int i = 0; i < num;++i)
	{
		_stocks.push_back(sts[i]);
	}
	_today_gain.resize(num);
	_gain_since_buy.resize(num);
	_values_now.resize(num);
	_parse_result.resize(num);
	_history_price.resize(num);
	_change_values.resize(num);
	_msg_levels.resize(num);
	_rest_money = resm;
}

float st_info::get_real_time_price()
{
	return _price;
}

void st_info::run_thread()
{
	_curl_handle = curl_easy_init();
	CURLcode t_code;
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_ERRORBUFFER, error_buffer);
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_NOPROGRESS, true);
	//CURLOPT_NOSIGNAL If onoff is 1, libcurl will not use any functions that install signal handlers or any functions that cause signals to be sent to the process. This option is here to allow multi-threaded unix applications to still set/use all timeout options etc, without risking getting signals.
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_NOSIGNAL, 1L); //在多线程场景下，若不设置CURLOPT_NOSIGNAL选项，可能会有“意外”的情况发生,skip all signal handling
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_TIMEOUT, 600);//in second
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_MAXREDIRS, 3);//查找次数，防止查找太深
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_FOLLOWLOCATION, 1L); //返回的头部中有Location(一般直接请求的url没找到)，则继续请求Location对应的数据,自动跳转
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_VERBOSE, 0L);//1在屏幕打印请求连接过程和返回http数据
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_WRITEFUNCTION, st_info::download_callback);
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_PROGRESSFUNCTION, st_info::progress_callback);
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_WRITEDATA, this);
	//A data pointer to pass to the write callback. If you use the CURLOPT_WRITEFUNCTION option, this is the pointer you'll get in that callback's 4th argument. If you don't use a write callback, you must make pointer a 'FILE *' (cast to 'void *') as libcurl will pass this to fwrite(3) when writing data.
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_CONNECTTIMEOUT, 3);//连接超时，这个数值如果设置太短可能导致数据请求不到就断开了
	//t_code = curl_easy_setopt(_curl_handle, CURLOPT_PROGRESSDATA, this);
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_HEADER, 0);
	t_code = curl_easy_setopt(_curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);

	curl_easy_setopt(_curl_handle, CURLOPT_HEADERFUNCTION, st_info::header_callback);
	curl_easy_setopt(_curl_handle, CURLOPT_HEADERDATA, this);

	//thread_function();
	std::thread t1(&st_info::thread_function, this);
	//t1.join();
	t1.detach();
}

void st_info::exit_thread()
{
	_need_exit = true;
}


void st_info::thread_function()
{
	static FILE* pfile = fopen("buy_record.txt", "a+");
	while (!_need_exit)
	{
		_result_str.clear();
		static char* base_char = "http://hq.sinajs.cn/list=";
		//http://quote.eastmoney.com/sz002166.html
		char req_str[2048];
		int snum = _stocks.size();
		strcpy(req_str, base_char);
		strcat(req_str, _stocks[0].code);
		for (int i = 1; i < snum; ++i)
		{
			strcat(req_str, ",");
			strcat(req_str, _stocks[i].code);
		}
		CURLcode code;
		code = curl_easy_setopt(_curl_handle, CURLOPT_URL, req_str);
		//code = curl_easy_setopt(_curl_handle, CURLOPT_URL, "http://c.csdnimg.cn/pubfooter/js/tracking.js");
		code = curl_easy_perform(_curl_handle);
		long retcode = 0;
		curl_easy_getinfo(_curl_handle, CURLINFO_RESPONSE_CODE, &retcode);
		//////////////////////////////////////////////////////////////////////////
		long header_size;
		code = curl_easy_getinfo(_curl_handle, CURLINFO_HEADER_SIZE, &header_size);
		long connect_code,http_version;
		code = curl_easy_getinfo(_curl_handle, CURLINFO_HTTP_CONNECTCODE, &connect_code);
		code = curl_easy_getinfo(_curl_handle, CURLINFO_HTTP_VERSION, &http_version);
		long tsocket;
		code = curl_easy_getinfo(_curl_handle, CURLINFO_LASTSOCKET, &tsocket);
		//CURLINFO_LOCAL_IP char** ip;
		//CURLINFO_LOCAL_PORT long* portp;
		//CURLINFO_NAMELOOKUP_TIME, double *timep;
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_NUM_CONNECTS, long *nump);
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_OS_ERRNO, long *errnop);
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_PRETRANSFER_TIME, double *timep);//get the time until the file transfer start
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_PRIMARY_IP, char **ip);
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_PRIMARY_PORT, long *portp);
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_PROTOCOL, long *p);
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_REDIRECT_URL, char **urlp);//get the URL a redirect would go to
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_REQUEST_SIZE, long *sizep);//get size of sent request
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_RESPONSE_CODE, long *codep);//get the last response code
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_SIZE_DOWNLOAD, double *dlp);//get the number of downloaded bytes
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_SIZE_UPLOAD, double *uploadp);//get the number of uploaded bytes
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_SPEED_DOWNLOAD, double *speed);//get download speed.
		//CURLcode curl_easy_getinfo(CURL *handle, CURLINFO_SPEED_UPLOAD, double *speed);//get upload speed
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_ACCEPT_ENCODING, char *enc);//enables automatic decompression of HTTP downloads

		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_BUFFERSIZE, long size);//set preferred receive buffer size
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_LOW_SPEED_LIMIT, long speedlimit);//set low speed limit in bytes per second
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_LOW_SPEED_TIME, long speedtime);//set low speed limit time period
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_PATH_AS_IS, long leaveit);//do not handle dot dot sequences
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_RANDOM_FILE, char *path);//specify a source for random data

		//read callback for data uploads
		//size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream);
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_READFUNCTION, read_callback);

		/* These are the return codes for the seek callbacks */
		//#define CURL_SEEKFUNC_OK       0
		//#define CURL_SEEKFUNC_FAIL     1 /* fail the entire transfer */
		//#define CURL_SEEKFUNC_CANTSEEK 2 /* tell libcurl seeking can't be done, so libcurl might try other means instead */
		//int seek_callback(void *userp, curl_off_t offset, int origin);
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_SEEKFUNCTION, seek_callback);

		//	typedef enum  {
		//	CURLSOCKTYPE_IPCXN,  /* socket created for a specific IP connection */
		//	CURLSOCKTYPE_ACCEPT, /* socket created by accept() call */
		//	CURLSOCKTYPE_LAST    /* never use */
		//} curlsocktype;
		//#define CURL_SOCKOPT_OK 0
		//#define CURL_SOCKOPT_ERROR 1 /* causes libcurl to abort and return CURLE_ABORTED_BY_CALLBACK */
		//#define CURL_SOCKOPT_ALREADY_CONNECTED 2
		//int sockopt_callback(void *clientp,curl_socket_t curlfd,curlsocktype purpose);
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);

		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_TCP_KEEPALIVE, long probe);//enable TCP keep-alive probing
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_TCP_KEEPIDLE, long delay);//set TCP keep-alive idle time wait
		//CURLcode curl_easy_setopt(CURL *handle, CURLOPT_UPLOAD, long upload);//enable data upload

		//CURLcode curl_easy_perform(CURL * easy_handle);// perform a blocking file transfer
		//curl_easy_recv
		//curl_easy_send
		//curl_easy_reset
		//////////////////////////////////////////////////////////////////////////
//#define macro_write_to_file
		Sleep(5000);
		//printf("\t Time \t\tcode\t\t价格\t涨跌\t最高\t最低\t振幅\n");
		if (_result_str.length() > 10)
		for (int i = 0; i < snum;++i)
		{
			parse_result(_result_str.c_str(), _stocks[i].code, _parse_result[i]);
			_history_price[i]._values[_current_record_pos] = _parse_result[i]._current_price;
			warning(_parse_result[i], 0.02);
			if (_need_judge && _stocks[i].num_hold > 0)
			{
				judge(_parse_result[i], i);
				change_status res = warn_change(_parse_result[i], i);
				HWND hd = GetDesktopWindow();
				char  msg[2] = { '0' + res, '\0' };
				switch (res)
				{
				case e_reach_loss:
					MessageBoxA(hd, "loss", _stocks[i].code, 0);
					break;
				case e_reach_sell:
					MessageBoxA(hd, "gain", _stocks[i].code, 0);
					break;
				case e_sudden_jump:
					MessageBoxA(hd, "jump", _stocks[i].code, 0);
					break;
				case e_sudden_up:
					MessageBoxA(hd, "up", _stocks[i].code, 0);
					break;
				case e_nothing:
				default:
					break;
				}
			}
			_values_now[i] = _parse_result[i]._current_price*_stocks[i].num_hold;
			_today_gain[i] = (_parse_result[i]._current_price - _parse_result[i]._last_close_price)*_stocks[i].num_hold;
			_gain_since_buy[i] = (_parse_result[i]._current_price - _stocks[i].value_init)*_stocks[i].num_hold;
			float zd = (_parse_result[i]._current_price / _parse_result[i]._last_close_price - 1.0f)*100.0f;
			float zf = _parse_result[i]._last_close_price > 0.1 ? (_parse_result[i]._highest_today_price - _parse_result[i]._lowest_today_price) / _parse_result[i]._last_close_price*100.0f : 0.0f;
#ifdef macro_write_to_file
			fprintf(pfile, "%d-%d %s %s %.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\t%.1f\t%.1f\t%.1f\n",// %d:%d:%d
				/*_parse_result[i]._year,*/ _parse_result[i]._month, _parse_result[i]._day,
				//_parse_result[i]._hour, _parse_result[i]._min, _parse_result[i]._second,
				_parse_result[i]._name, _stocks[i].code, fmod(_parse_result[i]._current_price, 10000),
				(_parse_result[i]._current_price / _parse_result[i]._last_close_price - 1.0f)*100.0f,
				//_parse_result[i]._buy_queue[0].num, _parse_result[i]._seal_queue[0].num, o
				fmod(_parse_result[i]._highest_today_price, 10000), fmod(_parse_result[i]._lowest_today_price, 10000),
				_parse_result[i]._last_close_price > 0.1 ? (_parse_result[i]._highest_today_price - _parse_result[i]._lowest_today_price) / _parse_result[i]._last_close_price*100.0f : 0.0f, i + 1
				, _today_gain[i], _gain_since_buy[i], _values_now[i]);
#else
			printf("%d-%d %s %.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\t%.1f\n",// %d:%d:%d %.1f 
				/*_parse_result[i]._year,*/ _parse_result[i]._month, _parse_result[i]._day,
				//_parse_result[i]._hour, _parse_result[i]._min, _parse_result[i]._second,
				/*_parse_result[i]._name,*/ _stocks[i].code, fmod(_parse_result[i]._current_price, 10000),
				zd,
				//_parse_result[i]._buy_queue[0].num, _parse_result[i]._seal_queue[0].num, o
				fmod(_parse_result[i]._highest_today_price, 10000), fmod(_parse_result[i]._lowest_today_price, 10000),
				zf, i + 1
				/*, _today_gain[i]*/, zd<-9.9 ? _parse_result[i]._seal_queue[0].num*_parse_result[i]._current_price / 10000.0f:_parse_result[i]._buy_queue[0].num*_parse_result[i]._current_price / 10000.0f);
#endif
				
		}
		_total_record++;
		_current_record_pos++;
		if (_current_record_pos >= HIS_NUM)
		{
			_current_record_pos = 0;
		}
		float today_gain_all = 0;
		float buy_gain_all = 0;
		float total_m = 0;
		for (int i = 0; i < _today_gain.size(); i++)
		{
			today_gain_all += _today_gain[i];
			total_m += _values_now[i];
			buy_gain_all += _gain_since_buy[i];
		}
#ifdef macro_write_to_file
		fprintf(pfile,
#else
		printf(
#endif
			"today result %.1f total_m %.1f today_gain %.1f all_gain %.1f\n", total_m, total_m+_rest_money, today_gain_all, buy_gain_all);
		fflush(pfile);
		printf("query parse finished.waiting for next query....\n");
	}
	fclose(pfile);
	printf("code %s thread is over.\n", _code);

	curl_easy_cleanup(_curl_handle);
}

size_t st_info::download_callback(void* pBuffer, size_t nSize, size_t nMemByte, void* pParam)
{
	if (NULL == pParam)
	{
		return 0;
	}
	st_info* pThis = (st_info*)pParam;
	std::string & str = pThis->_result_str;
	str.append((char*)pBuffer, nSize*nMemByte);
	//printf("current str is %s\n", str.c_str());
	return 0;
}

int st_info::xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	return 0;
}

int st_info::progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	return 0;
}

size_t st_info::upload_callback(void *pBuffer, size_t nSize, size_t nMemByte, FILE *pParam)
{
	return 0;
}

void st_info::parse_result(const char* result_str,const char* code, stock_record& aos)
{
	const char* t0 = strstr(result_str, code) + 2 + strlen(code);
	const char* t1 = strstr(t0, "\";");
	char  tmp[1024];
	memcpy(tmp, t0, t1 - t0);
	tmp[t1 - t0] = 0;
	sscanf(tmp, "%[^,],%f,%f,%f,%f,%f,%f,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d-%d-%d,%d:%d:%d",
		&aos._name, &aos._open_price, &aos._last_close_price, &aos._current_price, &aos._highest_today_price, &aos._lowest_today_price, &aos._buy_one_price, &aos._seal_one_price, &aos._deal_num, &aos._deal_money_num,
		&aos._buy_queue[0].num, &aos._buy_queue[0].price, &aos._buy_queue[1].num, &aos._buy_queue[1].price, &aos._buy_queue[2].num, &aos._buy_queue[2].price, &aos._buy_queue[3].num, &aos._buy_queue[3].price, &aos._buy_queue[4].num, &aos._buy_queue[4].price,
		&aos._seal_queue[0].num, &aos._seal_queue[0].price, &aos._seal_queue[1].num, &aos._seal_queue[1].price, &aos._seal_queue[2].num, &aos._seal_queue[2].price, &aos._seal_queue[3].num, &aos._seal_queue[3].price, &aos._seal_queue[4].num, &aos._seal_queue[4].price,
		&aos._year, &aos._month, &aos._day, &aos._hour, &aos._min, &aos._second);
}

void st_info::warning(stock_record& st, float a_s)
{
	return;//currently I do not use warning string,maybe I will use a box
	if (st._current_price > st._last_close_price*(1.0+a_s))
	{
		printf("warning,quickly deal it %s.\n", st._name);
	}
	else if (st._current_price < st._last_close_price*(1.0 - a_s))
	{
		if (st._current_price < st._last_close_price*(1.0 - a_s*2))
		{
			printf("fuck it throw it quickly now.%s", st._name);
		}
		else
		{
			printf("warning,quickly deal it %s.maybe you should throw it now\n", st._name);
		}
	}
}

void st_info::judge(const stock_record& st,int id)
{
	message_level message_show = message_null;
	if (st._current_price > st._last_close_price*1.05)
	{
		message_show = message_high_5;
	}
	else if (st._current_price > st._last_close_price*1.04)
	{
		message_show = message_high_4;
	}
	else if (st._current_price > st._last_close_price*1.03)
	{
		message_show = message_high_3;
	}
	else if (st._current_price > st._last_close_price*1.02)
	{
		message_show = message_high_2;
	}
	else if (st._current_price < st._last_close_price*0.96)
	{
		message_show = message_low_4;
	}
	else if (st._current_price < st._last_close_price*0.98)
	{
		message_show = message_low_2;
	}
	if (_msg_levels[id] == message_show && (message_show == message_high_5 || message_low_4==message_show) )
 	{
 		return;
 	}
	_msg_levels[id] = message_show;
	HWND hd = GetDesktopWindow();
	switch (message_show)
	{
	case message_null:
		break;
	case message_high_2:
		printf("+2\n");
		//MessageBoxA(hd, "+2", _stocks[id].code, 0);
		break;
	case message_high_3:
		printf("+3\n");
		MessageBoxA(hd, "+3", _stocks[id].code, 0);
		break;
	case message_high_4:
		printf("+4\n");
		//MessageBoxA(hd, "+4", _stocks[id].code, 0);
		break;
	case message_high_5:
		printf("+5\n");
		MessageBoxA(hd, "+5", _stocks[id].code, 0);
		break;
	case message_low_2:
		printf("-2\n");
		//MessageBoxA(hd, "-2", _stocks[id].code, 0);
		break;
	case message_low_4:
		printf("-4\n");
		MessageBoxA(hd, "-4", _stocks[id].code, 0);
		break;
	default:
		break;
	}
}

change_status st_info::warn_change(const stock_record& st, int id)
{
	//////////////////////////////////////////////////////////////////////////
	//for test
// 	_current_record_pos = HIS_NUM-1;
// 	_total_record = HIS_NUM;
// 	for (int i = 0; i < HIS_NUM; ++i)
// 	{
// 		_history_price[id]._values[i] = st._current_price * (1 - 0.001*i);
// 	}
	//////////////////////////////////////////////////////////////////////////
	int judge_num = _current_record_pos;
	float * t_value = _history_price[id]._values;
	if (_total_record > HIS_NUM)
	{
		t_value = new float[HIS_NUM];
		for (int i = 0; i < HIS_NUM; ++i)
		{
			t_value[i] = _history_price[id]._values[(_current_record_pos + i) % HIS_NUM];
		}
		judge_num = HIS_NUM-1;
	}
	change_status cs = e_nothing;
	//judge t_value
	if (t_value[judge_num] > _stocks[id].stop_profit)
	{
		cs = e_reach_sell;
	}
	else if (t_value[judge_num] < _stocks[id].stop_loss)
	{
		cs = e_reach_loss;
	}
	else if (t_value[judge_num] > t_value[judge_num /4] * 1.02)
	{
		cs = e_sudden_up;
	}
	else if (t_value[judge_num] < t_value[judge_num /4] * 0.98)
	{
		cs = e_sudden_jump;
	}
	if (_total_record > HIS_NUM)
	{
		delete[] t_value;
	}
	if (_change_values[id] == cs)
	{
		return e_nothing;
	}
	_change_values[id] = cs;
	return cs;
}

void st_info::use_judge(bool a_judge)
{
	_need_judge = a_judge;
}
