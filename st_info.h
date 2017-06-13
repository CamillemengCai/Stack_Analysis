#ifndef __st_info_h__
#define __st_info_h__

#include "curl/curl.h"
#include <string>
#include <vector>

typedef struct deal_info
{
	int num;
	float price;
};
typedef struct 
{
	char  code[12];
	int	  num_hold;
	float value_init;
	float stop_loss;
	float stop_profit;
}py_stock;
typedef struct stock_record 
{
	char	_name[12];
	float	_open_price;
	float	_last_close_price;
	float	_current_price;
	float	_highest_today_price;
	float	_lowest_today_price;
	float	_buy_one_price;
	float	_seal_one_price;
	int		_deal_num;
	float	_deal_money_num;
	deal_info	_buy_queue[5];
	deal_info	_seal_queue[5];
	int			_year, _month, _day;
	int			_hour, _min, _second;
};
typedef struct record_pair
{
	int		_time;//totally 4*60  = 240,in minutes
	float	_price;
};
typedef struct day_record 
{
	char	_code[8];
	char	_name[12];
	int		_day;//since 1990.1.1,added per day
	std::vector<record_pair>	_price_records;
};
#define HIS_NUM 64 //record about 10 minutes
typedef struct 
{
	float	_values[HIS_NUM];
}data_history;
enum change_status
{
	e_nothing,
	e_sudden_up,
	e_sudden_jump,
	e_reach_sell,
	e_reach_loss,
	e_status_count
};
class st_info
{
public:
	enum message_level
	{
		message_null,
		message_high_2,
		message_high_3,
		message_high_4,
		message_high_5,
		message_low_2,
		message_low_4
	};
public:
	st_info();
	~st_info();
public:
	void set_stock_id(py_stock* sts, int num, float resm);
	void run_thread();
	void exit_thread();
	float get_real_time_price();
	void use_judge(bool a_judge);
private:
	void thread_function();
	static size_t header_callback(void *ptr, size_t size, size_t nmemb, void *stream){ return size*nmemb; }
	static size_t download_callback(void* pBuffer, size_t nSize, size_t nMemByte, void* pParam);
	static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

	/* curl version > 7.32 , use function below  for CURLOPT_PROGRESSFUNCTION */
	static int    progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	static size_t upload_callback(void *pBuffer, size_t nSize, size_t nMemByte, FILE *pParam);
	void parse_result(const char* result_str, const char* code, stock_record& aos);
	void warning(stock_record& st,float a_s);
	void judge(const stock_record& st,int id);
	change_status  warn_change(const stock_record& st, int id);
public:
	std::vector<float>	_prices;
	std::vector<float>	_today_gain;
	std::vector<float>	_gain_since_buy;
	std::vector<float>	_values_now;
	std::vector<float>	_today_values;
	std::vector<py_stock>	_stocks;
	std::vector<stock_record>	_parse_result;
	std::vector<data_history>	_history_price;
	std::vector<change_status>	_change_values;
	std::vector<message_level>	_msg_levels;
	int					_current_record_pos;
	int					_total_record;
	CURL*   _curl_handle;
	float	_price;
	float	_rest_money;
	std::string _result_str;
	char	_code[12];
	std::string	_back_str;
	bool	_need_exit;
	bool	_need_judge;
};

#endif
