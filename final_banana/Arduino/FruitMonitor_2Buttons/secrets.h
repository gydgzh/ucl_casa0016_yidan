#ifndef SECRETS_H
#define SECRETS_H

// Application EUI / JoinEUI  (LSB)
const char TTN_APP_EUI[] = "0000000000000000";

// Device EUI (LSB) —— 主要是给你参考，真正 join 用的是板子里的硬件 DevEUI
const char TTN_DEV_EUI[] = "A8610A35392D7308";

// Application Key / AppKey (MSB)
// 和控制台里的一模一样，中间不要空格
const char TTN_APP_KEY[] = "FDDE707BDA0DEA9A882F0EDB379129E5";

#endif

