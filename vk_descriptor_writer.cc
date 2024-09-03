#include "vk_descriptor_writer.h"

void vk_descriptor_writer_write_image(DescriptorWriter *writer, uint32_t binding, VkDescriptorType type,
                                      VkSampler sampler, VkImageView image_view, VkImageLayout layout) {
    VkDescriptorImageInfo image_info = {
            .sampler = sampler,
            .imageView = image_view,
            .imageLayout = layout
    };
    writer->image_infos.push_back(image_info);

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &writer->image_infos.back();

    writer->writes.push_back(write);
}

void vk_descriptor_writer_write_buffer(DescriptorWriter *writer, uint32_t binding, VkDescriptorType type,
                                       VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
    VkDescriptorBufferInfo buffer_info = {
            .buffer = buffer,
            .offset = offset,
            .range = size
    };
    writer->buffer_infos.push_back(buffer_info);

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &writer->buffer_infos.back();

    writer->writes.push_back(write);
}

void vk_descriptor_writer_clear(DescriptorWriter *writer) {
    writer->image_infos.clear();
    writer->buffer_infos.clear();
    writer->writes.clear();
}

void
vk_descriptor_writer_update_descriptor_set(DescriptorWriter *writer, VkDevice device, VkDescriptorSet descriptor_set) {
    for (VkWriteDescriptorSet &write: writer->writes) {
        write.dstSet = descriptor_set;
    }
    vkUpdateDescriptorSets(device, writer->writes.size(), writer->writes.data(), 0, nullptr);
}
