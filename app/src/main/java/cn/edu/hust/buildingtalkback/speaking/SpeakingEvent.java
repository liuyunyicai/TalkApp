package cn.edu.hust.buildingtalkback.speaking;

/**
 * Created by admin on 2016/1/15.
 * 用以与Speaking之间传递消息
 */
public class SpeakingEvent {

    // 挂断成功标志
    public static final int RING_HANGOUT_SUCCESS = 1;

    private int action;
    private String data;

    public SpeakingEvent(int action) {
        this.action = action;
    }

    public SpeakingEvent(int action, String data) {
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
