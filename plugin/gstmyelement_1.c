#include "gst/gst.h"
#include "gst/base/gstbasetransform.h"

#define PACKAGE "MYELEMENT"
/* 插件元信息定义（必填） */
#define GST_TYPE_MY_ELEMENT (gst_my_element_get_type())
#define GST_MY_ELEMENT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MY_ELEMENT, GstMyElement))
#define GST_MY_ELEMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MY_ELEMENT, GstMyElementClass))
#define GST_IS_MY_ELEMENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MY_ELEMENT))
#define GST_IS_MY_ELEMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MY_ELEMENT))

/* 自定义元素结构体（存储元素的私有数据） */
typedef struct _GstMyElement
{
    GstBaseTransform parent; // 继承 GstBaseTransform（简化数据处理逻辑）
    // 这里可以添加你的自定义参数/状态
    gboolean enable_log; // 示例：是否开启日志
} GstMyElement;

/* 自定义元素类结构体 */
typedef struct _GstMyElementClass
{
    GstBaseTransformClass parent_class; // 父类
                                        // 可以添加类级别的方法/回调
} GstMyElementClass;

/* 函数声明 */
static void gst_my_element_set_property(GObject *object, guint prop_id,
                                        const GValue *value, GParamSpec *pspec);
static void gst_my_element_get_property(GObject *object, guint prop_id,
                                        GValue *value, GParamSpec *pspec);
static gboolean gst_my_element_transform_ip(GstBaseTransform *trans, GstBuffer *buf);

/* 注册元素的属性（示例：enable-log） */
enum
{
    PROP_0,
    PROP_ENABLE_LOG,
};
/* 第一步：获取元素类型 */
G_DEFINE_TYPE(GstMyElement, gst_my_element, GST_TYPE_BASE_TRANSFORM);
/* 第二步：初始化元素实例 */
static void gst_my_element_init(GstMyElement *element)
{
    // 设置元素的默认参数（透传模式，无需复制缓冲区）
    // 在实例初始化时设置 BaseTransform 属性
    GstBaseTransform *base = GST_BASE_TRANSFORM(element);
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(base), TRUE);
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(base), TRUE);

    // 初始化自定义属性
    element->enable_log = TRUE;
    g_print("MyElement instance initialized\n");
}

/* 第三步：初始化元素类 */
static void gst_my_element_class_init(GstMyElementClass *klass)
{

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass); // 重要！
    // 绑定属性的 get/set 方法
    gobject_class->set_property = gst_my_element_set_property;
    gobject_class->get_property = gst_my_element_get_property;
    // 注册自定义属性：enable-log（布尔类型，可读写）
    g_object_class_install_property(
        gobject_class, PROP_ENABLE_LOG,
        g_param_spec_boolean("enable-log", "Enable Log",
                             "Whether to enable data processing log",
                             TRUE, G_PARAM_READWRITE));

    // 绑定核心数据处理方法（透传数据，可修改为自定义逻辑）
    base_transform_class->transform_ip = gst_my_element_transform_ip;

    // ============ 必须的元素配置 ============

    // 1. 设置元素元数据（必须！）
    gst_element_class_set_details_simple(element_class,
                                         "My Custom Element",                   // 长名称
                                         "Filter/Effect/Video",                 // 分类
                                         "A simple custom GStreamer element",   // 描述
                                         "Your Name <your.email@example.com>"); // 作者

    // 2. 创建 Pad 模板
    static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
        "sink",             // Pad 名称
        GST_PAD_SINK,       // Pad 方向
        GST_PAD_ALWAYS,     // Pad 可用性
        GST_STATIC_CAPS_ANY // Pad 能力
    );

    static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
        "src",              // Pad 名称
        GST_PAD_SRC,        // Pad 方向
        GST_PAD_ALWAYS,     // Pad 可用性
        GST_STATIC_CAPS_ANY // Pad 能力
    );
    // 3. 将 Pad 模板添加到元素类
    // gst_element_class_add_static_pad_template(element_class, &sink_template);
    // gst_element_class_add_static_pad_template(element_class, &src_template);
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&gst_dsexample_src_template));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&gst_dsexample_sink_template));
}

/* 第四步：实现属性的 set 方法 */
static void gst_my_element_set_property(GObject *object, guint prop_id,
                                        const GValue *value, GParamSpec *pspec)
{
    GstMyElement *element = GST_MY_ELEMENT(object);

    switch (prop_id)
    {
    case PROP_ENABLE_LOG:
        element->enable_log = g_value_get_boolean(value);
        g_print("Enable log set to: %s\n", element->enable_log ? "TRUE" : "FALSE");
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/* 第五步：实现属性的 get 方法 */
static void gst_my_element_get_property(GObject *object, guint prop_id,
                                        GValue *value, GParamSpec *pspec)
{
    GstMyElement *element = GST_MY_ELEMENT(object);

    switch (prop_id)
    {
    case PROP_ENABLE_LOG:
        g_value_set_boolean(value, element->enable_log);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/* 第六步：核心数据处理逻辑（IP模式：直接修改缓冲区，无需复制） */
static gboolean gst_my_element_transform_ip(GstBaseTransform *trans, GstBuffer *buf)
{
    GstMyElement *element = GST_MY_ELEMENT(trans);

    // 如果开启日志，打印缓冲区信息
    // if (element->enable_log)
    // {
    guint64 pts = GST_BUFFER_PTS(buf);     // 获取缓冲区的时间戳
    gsize size = gst_buffer_get_size(buf); // 获取缓冲区大小
    g_print("MyElement processing buffer: PTS = %" GST_TIME_FORMAT ", Size = %zu bytes\n", GST_TIME_ARGS(pts), size);
    // }

    // 这里可以添加自定义数据处理逻辑（比如修改缓冲区内容）
    // 示例：不修改数据，直接返回成功
    return TRUE;
}

/* 第七步：注册插件和元素 */
static gboolean my_element_plugin_init(GstPlugin *plugin)
{
    // 直接注册元素并返回结果
    return gst_element_register(plugin, "myelement", GST_RANK_NONE, GST_TYPE_MY_ELEMENT);
}

/* 插件元信息（GStreamer 固定格式） */
GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    myelement,                                  // 插件名称（小写，唯一）
    "A simple GStreamer custom element plugin", // 插件描述
    my_element_plugin_init,                     // 插件初始化函数
    "1.0",                                      // 插件版本
    "LGPL",                                     // 许可证
    "GStreamer Tutorial",                       // 插件名称（全称）
    "https://gstreamer.freedesktop.org/"        // 插件主页
)
/* 编译时的主函数（可选，用于测试） */
#ifdef BUILD_TEST
int main(int argc, char *argv[])
{
    // 初始化 GStreamer
    gst_init(&argc, &argv);

    // 注册插件（模拟插件加载）
    if (!my_element_plugin_init(NULL))
    {
        g_error("Failed to register myelement plugin\n");
        return 1;
    }

    // 创建元素实例
    GstElement *element = gst_element_factory_make("myelement", "myelement0");
    if (!element)
    {
        g_error("Failed to create myelement instance\n");
        return 1;
    }

    // 设置元素属性
    g_object_set(element, "enable-log", FALSE, NULL);

    // 打印元素信息
    g_print("MyElement plugin test success\n");

    // 释放资源
    gst_object_unref(element);
    gst_deinit();

    return 0;
}
#endif