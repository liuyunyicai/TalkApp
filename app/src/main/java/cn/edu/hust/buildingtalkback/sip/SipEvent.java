package cn.edu.hust.buildingtalkback.sip;

/**
 * Created by admin on 2015/12/30.
 */

/** 用以通知SipService进行SIP相关操作 **/
public class SipEvent {
    public static final int SIP_REGISTER = 0;    // SIP注册
    public static final int SIP_UNREGISTER = 1;  // SIP解注册

    public static final int SIP_REGISTER_SUCCESS = 2;  // SIP注册成功
    public static final int SIP_REGISTER_FAIL = 3;     // SIP注册成功

    // ========== 拨打电话 ======= //
    public static final int SIP_CALLIN_RINGING = 4;  // 呼入电话
    public static final int SIP_CALLOUT_RINGING = 5; // 呼出电话
    public static final int SIP_CALLIN_BUSY = 6;     // 正在通话中
    public static final int SIP_CALLIN_HANGOUT = 7;  // 结束通话（呼入）
    public static final int SIP_CALLOUT_HANGOUT = 8; // 结束通话（呼出）


    private int sipAction; // SIP消息标志

    private String data; // 待连接服务器IP地址

    public SipEvent(int sipAction) {
        this(sipAction, "!");
    }

    public SipEvent(int sipAction, String data) {
        this.sipAction = sipAction;
        this.data  = data;
    }

    public int getSipAction() {
        return sipAction;
    }

    public void setSipAction(int sipAction) {
        this.sipAction = sipAction;
    }

    public String getData() {
        return data;
    }

    public void setData(String data) {
        this.data = data;
    }
}
