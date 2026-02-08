#include "gst/gst.h"
#include <iostream>

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    std::cout << "=== Testing My Plugin ===" << std::endl;

    // 1. 检查插件是否加载
    GstPlugin *plugin = gst_registry_find_plugin(gst_registry_get(), "myelement");
    if (!plugin)
    {
        std::cout << "Plugin 'myelement' NOT found!" << std::endl;

        // 检查所有插件
        GList *plugins = gst_registry_get_plugin_list(gst_registry_get());
        for (GList *l = plugins; l; l = l->next)
        {
            GstPlugin *p = (GstPlugin *)l->data;
            std::cout << "Found plugin: " << gst_plugin_get_name(p) << std::endl;
        }
        g_list_free(plugins);
    }
    else
    {
        std::cout << "Plugin found: " << gst_plugin_get_name(plugin) << std::endl;
        gst_object_unref(plugin);
    }

    // 2. 尝试创建元素
    std::cout << "\n=== Trying to create element ===" << std::endl;
    GstElementFactory *factory = gst_element_factory_find("myelement");
    if (!factory)
    {
        std::cout << "Element factory NOT found!" << std::endl;

        // 检查所有元素工厂
        GList *factories = gst_element_factory_list_get_elements(
            GST_ELEMENT_FACTORY_TYPE_ANY, GST_RANK_NONE);
        for (GList *l = factories; l; l = l->next)
        {
            GstElementFactory *f = (GstElementFactory *)l->data;
            std::cout << "Found factory: " << gst_element_factory_get_longname(f) << std::endl;
        }
        g_list_free(factories);
    }
    else
    {
        std::cout << "Element factory found!" << std::endl;

        GstElement *element = gst_element_factory_create(factory, "test-element");
        if (!element)
        {
            std::cout << "FAILED to create element!" << std::endl;
        }
        else
        {
            std::cout << "SUCCESS: Element created!" << std::endl;
            gst_object_unref(element);
        }
        gst_object_unref(factory);
    }

    return 0;
}