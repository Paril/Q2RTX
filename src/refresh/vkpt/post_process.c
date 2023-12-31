#include "vkpt.h"

static VkDescriptorSet postprocess_descriptor_set;
static VkPipelineLayout postprocess_pipeline_layout;
static VkPipeline postprocess_pipeline;

VkResult vkpt_postprocess_initialize()
{
	VkResult result = VK_SUCCESS;

	VkDescriptorSetLayout desc_set_layouts[] = { 
		qvk.desc_set_layout_ubo,
		qvk.desc_set_layout_textures,
	};


	CREATE_PIPELINE_LAYOUT(qvk.device, &postprocess_pipeline_layout,
		.setLayoutCount = LENGTH(desc_set_layouts),
		.pSetLayouts = desc_set_layouts,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
	);

	ATTACH_LABEL_VARIABLE(postprocess_pipeline_layout, PIPELINE_LAYOUT);

	return VK_SUCCESS;
}

VkResult vkpt_postprocess_create_pipelines()
{
	// Create compute pipeline

	VkComputePipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = SHADER_STAGE(QVK_MOD_POST_PROCESS_COMP, VK_SHADER_STAGE_COMPUTE_BIT),
		.layout = postprocess_pipeline_layout,
	};

	_VK(vkCreateComputePipelines(qvk.device, 0, 1, &pipeline_info, 0, &postprocess_pipeline));

	return VK_SUCCESS;
}

#define BARRIER_COMPUTE(cmd_buf, img) \
	do { \
		VkImageSubresourceRange subresource_range = { \
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, \
			.baseMipLevel   = 0, \
			.levelCount     = 1, \
			.baseArrayLayer = 0, \
			.layerCount     = 1 \
		}; \
		IMAGE_BARRIER(cmd_buf, \
				.image            = img, \
				.subresourceRange = subresource_range, \
				.srcAccessMask    = VK_ACCESS_SHADER_WRITE_BIT, \
				.dstAccessMask    = VK_ACCESS_SHADER_READ_BIT, \
				.oldLayout        = VK_IMAGE_LAYOUT_GENERAL, \
				.newLayout        = VK_IMAGE_LAYOUT_GENERAL, \
		); \
	} while(0)

VkResult vkpt_postprocess_record_cmd_buffer(VkCommandBuffer cmd_buf)
{
	VkDescriptorSet desc_sets[] = {
		qvk.desc_set_ubo,
		qvk_get_current_desc_set_textures(),
	};

	BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_TAA_OUTPUT]);

	// Record instructions to run the compute shader that updates the histogram.
	vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, postprocess_pipeline);
	vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
		postprocess_pipeline_layout, 0, LENGTH(desc_sets), desc_sets, 0, 0);

	vkCmdDispatch(cmd_buf,
		(qvk.extent_taa_output.width + 15) / 16,
		(qvk.extent_taa_output.height + 15) / 16,
		1);

	BARRIER_COMPUTE(cmd_buf, qvk.images[VKPT_IMG_POST_PROCESS_INPUT]);

	return VK_SUCCESS;
}

VkResult vkpt_postprocess_destroy()
{
	vkDestroyPipelineLayout(qvk.device, postprocess_pipeline_layout, NULL);
	postprocess_pipeline_layout = 0;
	return VK_SUCCESS;
}

VkResult vkpt_postprocess_destroy_pipeline()
{
	vkDestroyPipeline(qvk.device, postprocess_pipeline, NULL);
	postprocess_pipeline = 0;
	return VK_SUCCESS;
}