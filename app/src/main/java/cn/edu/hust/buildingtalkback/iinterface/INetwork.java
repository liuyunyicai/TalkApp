package cn.edu.hust.buildingtalkback.iinterface;

/** 提供NetWORK网络连接的相关接口 **/
public interface INetwork {
	// 等待客户端连接
	int waitForClient();

	// 开启客户端
	int startClient();

	// 关闭网络连接
	void stopNetwork();

}
