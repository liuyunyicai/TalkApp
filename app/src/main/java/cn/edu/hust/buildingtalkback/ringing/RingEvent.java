package cn.edu.hust.buildingtalkback.ringing;

/**
 * Created by admin on 2016/1/14.
 */
public class RingEvent {
    public static final int RING_HANGOUT_SUCCESS = 1; // 成功挂断电话
    public static final int RING_CALLIN_RECV_SUCCESS = 2; // 成功接听电话(对于Callin用户)
    public static final int RING_CALLOUT_RECV_SUCCESS = 3; // 成功接听电话（对于Callout用户）

    private int action; // 操作类型
    private String data;

    public RingEvent(int action) {
        this.action = action;
    }

    public RingEvent(int action, String data) {
        this.action = action;
        this.data = data;
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
