
protocol log{
@namespace("com.mtty.protocol")
record GeoInfo{
    // 纬度
    string latitude;
    // 经度
    string longitude;
    // 国家编码
    int country;
    // 省份编码
    int province;
    // 城市编码
    int city;
    // 区域编码
    int district;
    //街道编码
    int street;
}

@namespace("com.mtty.protocol")
record AdInfo{
    // 地域Id,可能为空,取决于接收到的链接
    string areaId;
    // 广告主Id,可能为空,取决于接收到的链接
    long advId;
    // 推广计划Id,可能为空,取决于接收到的链接
    long cpid;
    // 推广单元Id,可能为空,取决于接收到的链接
    long sid;
    // 创意Id,可能为空,取决于接收到的链接
    long bannerId;
    // 点击Id,可能为空,取决于接收到的链接
    string clickId;
    // 广告交换商Id,可能为空,取决于接收到的链接
    int adxid ;
    // 网站Id,可能为空,取决于接收到的链接
    long mid ;
    // 频道Id,可能为空,取决于接收到的链接
    long cid ;
    // 广告位Id,可能为空,取决于接收到的链接
    string pid ;
    // 曝光Id,可能为空,取决于接收到的链接
    string imp_id;
    // 广告的落地页面,click模块非空
    string landingUrl;
    // 在点击模块和曝光模块中使用，表示成本
    int cost;
    // 在点击模块和曝光模块中使用，表示实际扣广告主的钱
    int bidPrice;
    // 广告交换商的pid,暂时和pid维持一样
    string adxpid ;
    // 广告交换商的uid
    string adxuid;
    // 竞价模块预计的结算价格,对应生成链接里的b参数，在点击和曝光模块转化为bidPrice字段
    int offerPrice;
    // 竞价模块请求的广告size
    string bidSize;
    // 最终有效出价类型
    int priceType;
    // 对应产品包
    int ppid;
    // 订单id
    long orderId;
    // 广告位底价
    int bidBasePrice;
    // 真实的出价价格,不乘利润率,通过高级出价器和CTR计算得来的
    int bidEcpmPrice;
}

@namespace("com.mtty.protocol")
enum LogPhaseType {
    BID,SHOW,VIEW,CLICK,TRACK,MAPPING
}

@namespace("com.mtty.protocol")
record IPInfo{
    //非空字段
    int ipv4;
    //可能为空
    array<int> ipv6;
    //可能为空
    string proxy;
}

@namespace("com.mtty.protocol")
record UserInfo{
    // 可能为0,取决于客户端或第三方渠道获取到的信息
    int age;
    // 可能为0,取决于客户端或第三方渠道获取到的信息
    int sex;
    // 可能为0,取决于客户端或第三方渠道获取到的信息
    int interest;
}

@namespace("com.mtty.protocol")
record TraceInfo{
    // 版本号
    string version;
    // 1 PC端  2 移动端
    string deviceType;
    // c模块生成
    string sourceid;
    // 用户ID/订单ID
    string tag1;
    // 用户名/商品名
    string tag2;
    // 商品ID
    string tag3;
    // 商品价格
    string tag4;
    // 商品数量
    string tag5;
    // 订单总金额
    string tag6;
    // 商品图片地址
    string tag7;
    // 商品URL地址
    string tag8;
    // 
    string tag9;
    // 
    string tag10;
}

@namespace("com.mtty.protocol")
record DeviceInfo{
    // 流量类型:PC或移动
    int flowType;
    // 移动设备类型:iphone/AndroidPhone/iPad/等
    int deviceType;
    // 网络环境:4G/wifi等
    int netWork;
    // 操作系统
    int os;
    // 设备ID号：一般为移动设备ID号，IOS取IDFA，安卓优先使用androidid，其次为IMEI
    string deviceID;
    // 设备制造商
    string make;
}

}
