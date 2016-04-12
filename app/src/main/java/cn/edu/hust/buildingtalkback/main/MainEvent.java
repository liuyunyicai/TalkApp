package cn.edu.hust.buildingtalkback.main;

/**
 * Created by admin on 2015/12/30.
 */
// 用以向MainActivity通知的Event
public class MainEvent {
    public static final int SIP_REGISTER_SUCCESS = 1;
    public static final int SIP_REGISTER_FAIL = 2;
    public static final int SIP_REGISTER_STATE_CHANGE = 3;

    private int action;
    private String data;

    public MainEvent(int action) {
        this.action = action;
    }

    public MainEvent(int action, String data) {
        this.action = action;
        this.data   = data;
    }

    public int getAction() {
        return action;
    }

    public void setAction(int action) {
        this.action = action;
    }

    public String getData() {
        return data;
    }

    public void setData(String data) {
        this.data = data;
    }
}
