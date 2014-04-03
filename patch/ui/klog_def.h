#undef NHLOG_CHK_AND_CALL_TMP
#define NHLOG_CHK_AND_CALL_TMP(mask, indi, modu, file, func, line, fmt, ...) do { \
	static int VAR_UNUSED __nhl_ver_sav = -1; \
	static int VAR_UNUSED __nhl_func_name_id = -1; \
	static int VAR_UNUSED __nhl_mask = 0; \
	static int VAR_UNUSED __nhl_modu_name_id_tmp_ = -1; \
	int VAR_UNUSED __nhl_ver_get = nhlog_touches(); \
	if (__nhl_ver_get > __nhl_ver_sav) { \
		__nhl_ver_sav = __nhl_ver_get; \
		if (__nhl_file_name_id_ == -1) { \
			__nhl_file_name_ = nhlog_get_name_part(file); \
			__nhl_file_name_id_ = nhlog_file_name_add(__nhl_file_name_); \
		} \
		if (__nhl_prog_name_id_ == -1) { \
			__nhl_prog_name_ = nhlog_get_prog_name(); \
			__nhl_prog_name_id_ = nhlog_prog_name_add(__nhl_prog_name_); \
		} \
		if (__nhl_modu_name_id_tmp_ == -1) { \
			__nhl_modu_name_id_tmp_ = nhlog_modu_name_add(modu); \
		} \
		if (__nhl_func_name_id == -1) { \
			__nhl_func_name_id = nhlog_func_name_add(func); \
		} \
		__nhl_mask = nhlog_calc_mask(__nhl_prog_name_id_, __nhl_modu_name_id_tmp_, __nhl_file_name_id_, __nhl_func_name_id, line, (int)getpid()); \
		if (!(__nhl_mask & (mask))) { \
			__nhl_mask = 0; \
		} \
	} \
	if (__nhl_mask) { \
		nhlog_f((indi), __nhl_mask, __nhl_prog_name_, modu, __nhl_file_name_, func, line, fmt, ##__VA_ARGS__); \
	} \
} while (0)

/* NHLOG_CHK_AND_CALL_TMP(NHLOG_INFO, 'L', "UI", __FILE__, __func__, __LINE__, "modua: aaaaaaaaaaa %d\n", i); */

