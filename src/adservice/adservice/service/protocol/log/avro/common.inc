
protocol log{
@namespace("com.mtty.protocol")
record GeoInfo{
    // 纬度，暂时无效
    string latitude;
    // 经度，暂时无效
    string longitude;
    // 国家编码,show,click有效，bid有出价时有效
    int country;
    // 省份编码,show,click有效，bid有出价时有效
    int province;
    // 城市编码,show,click有效，bid有出价时有效
    int city;
    // 区域编码,暂时无效
    int district;
    //街道编码,暂时无效
    int street;
}

@namespace("com.mtty.protocol")
record AdInfo{
    // 地域Id,show，click有效,ssp 无效
    string areaId;
    // 广告主Id,bid有出价时有效，show,click,trace有效
    long advId;
    // 推广计划Id,暂时无效
    long cpid;
    // 推广单元Id,bid有出价时有效，show,click有效
    long sid;
    // 创意Id,bid有出价时有效，show,click有效
    long bannerId;
    // 点击Id,暂时无效
    string clickId;
    // 广告交换商Id,bid有出价时有效，show，click,trace有效
    int adxid ;
    // 网站Id,bid有出价时有效,show,click,trace有效
    long mid ;
    // 频道Id,暂时无效
    long cid ;
    // 广告位Id,bid,show，click有效
    string pid ;
    // 曝光Id,bid有出价时有效,show,click有效
    string imp_id;
    // 广告的落地页面,click模块有效
    string landingUrl;
    // 在点击模块和曝光模块中使用，表示成本
    int cost;
    // 在点击模块和曝光模块中使用，表示实际扣广告主的钱
    int bidPrice;
    // 广告交换商的pid,bid,show，click有效
    string adxpid ;
    // 广告交换商的uid,bid模块暂时只有tanx有效
    string adxuid;
    // bid出价时有效，竞价模块预计的结算价格,对应生成链接里的b参数，在点击和曝光模块转化为bidPrice字段
    int offerPrice;
    // 竞价模块请求的广告size，bid模块有效
    string bidSize;
    // 最终有效出价类型,bid出价时,click,show，trace模块有效
    int priceType;
    // 对应产品包,bid出价时,click,show,trace模块有效
    int ppid;
    // 订单id,bid出价时,click，show,trace模块有效
    long orderId;
    // 广告位底价,bid有效
    int bidBasePrice;
    // 真实的出价价格,不乘利润率,通过高级出价器和CTR计算得来的,bid有效
    int bidEcpmPrice;
}

@namespace("com.mtty.protocol")
enum LogPhaseType {
    BID,SHOW,VIEW,CLICK,TRACK,MAPPING
}

@namespace("com.mtty.protocol")
record IPInfo{
    //无效
    int ipv4;
    //无效
    array<int> ipv6;
    //有效的ip字符串,show,click从http头部取，bid模块从包中取，包中没有从http头部取
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
    // 设备制造商，暂时无效
    string make;
}

}
