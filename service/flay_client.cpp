#include "backends/p4tools/modules/flay/service/flay_client.h"

#include "backends/p4tools/modules/flay/lib/logging.h"

namespace P4Tools::Flay {

FlayClient::FlayClient(const std::shared_ptr<grpc::Channel> &channel)
    : stub_(p4::v1::P4Runtime::NewStub(channel)) {}

bool FlayClient::sendWriteRequest(const p4::v1::Entity &entity, const p4::v1::Update_Type &type) {
    grpc::ClientContext context;
    p4::v1::WriteRequest request;
    printInfo("Sending update...\n");

    auto *update = request.add_updates();
    *update->mutable_entity() = entity;
    update->set_type(type);

    p4::v1::WriteResponse response;
    stub_->Write(&context, request, &response);
    return true;
}

std::optional<p4::v1::Entity> FlayClient::parseEntity(const std::string &message) {
    p4::v1::Entity entity;
    if (google::protobuf::TextFormat::ParseFromString(message, &entity)) {
        printInfo("Parsed entity: %1%", entity.DebugString());
    } else {
        std::cerr << "Message not valid (partial content: " << entity.ShortDebugString() << ")\n";
        return std::nullopt;
    }
    return entity;
}

}  // namespace P4Tools::Flay
