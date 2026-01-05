#include "Camera.hpp"

namespace gps {

    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp)
    {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;

        this->cameraFrontDirection = glm::normalize(this->cameraPosition - this->cameraTarget);

        this->cameraRightDirection = glm::normalize(glm::cross(worldUp, cameraFrontDirection));

        this->cameraUpDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraRightDirection));
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(this->cameraPosition, this->cameraFrontDirection + this->cameraPosition, this->cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        glm::vec3 moveVector(0.0f);

        switch (direction)
        {
        case MOVE_DIRECTION::MOVE_FORWARD:
            moveVector = this->cameraFrontDirection * speed;
            break;
        case MOVE_DIRECTION::MOVE_BACKWARD:
            moveVector = -this->cameraFrontDirection * speed;
            break;
        case MOVE_DIRECTION::MOVE_RIGHT:
            moveVector = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection)) * speed;
            break;
        case MOVE_DIRECTION::MOVE_LEFT:
            moveVector = -glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection)) * speed;
            break;
        }

        this->cameraPosition += moveVector;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->cameraFrontDirection = glm::normalize(direction);
    }
}
