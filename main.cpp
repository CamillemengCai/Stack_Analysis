

#include "st_info.h"

int main()
{
	st_info sti;
	sti.use_judge(true);

		py_stock values[] =
		{
			{ "sh000001"/*��*/, 0, 0, 3090, 3150 }
			//,{ "sz399001"/*��*/, 0, 0,9700,10100 }
			//,{ "sz399006"/*��*/, 0, 0,0,0 }
			, { "sz399005"/*��С*/, 0, 0, 6400, 6800 }
			, { "sh600550"/*��С*/, 400, 12.3, 12, 13 }
		};

		int tt00 = sizeof(values) / sizeof(py_stock);
		sti.set_stock_id(values, tt00, 560);

			sti.run_thread();
			while (true)
			{

			}

	return 0;
}