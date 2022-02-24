from rlbot.agents.base_agent import BaseAgent, SimpleControllerState
from rlbot.messages.flat.QuickChatSelection import QuickChatSelection
from rlbot.utils.structures.game_data_struct import GameTickPacket

from util.ball_prediction_analysis import find_slice_at_time
from util.boost_pad_tracker import BoostPadTracker
from util.drive import limit_to_safe_range
from util.sequence import Sequence, ControlStep
from util.vec import Vec3

import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2' 

import numpy as np
from nn_model import NN_Model
from operator import attrgetter
from copy import deepcopy
import math

class MyBot(BaseAgent):

    def __init__(self, name, team, index):
        super().__init__(name, team, index)
        self.active_sequence: Sequence = None
        self.boost_pad_tracker = BoostPadTracker()
        self.model = NN_Model()
        self.model.load_model('2022-02-18_00-43')
        
        # Assume bot is always blue (negative y region is their region)
        self.team_factor = 1 if self.team == 0 else -1

        # Max constraints for normalizing data
        self.max_region = Vec3(4096, 5120 + 880, 2044)
        self.max_velocity = Vec3(6000, 6000, 6000)
        self.max_ang_velocity = Vec3(math.pi/2, math.pi, math.pi)

    def initialize_agent(self):
        # Set up information about the boost pads now that the game is active and the info is available
        self.boost_pad_tracker.initialize_boosts(self.get_field_info())

    def get_output(self, packet: GameTickPacket) -> SimpleControllerState:
        """
        This function will be called by the framework many times per second. This is where you can
        see the motion of the ball, etc. and return controls to drive your car.
        """
        inputs = []

        # Keep our boost pad info updated with which pads are currently active
        self.boost_pad_tracker.update_boost_status(packet)

        # This is good to keep at the beginning of get_output. It will allow you to continue
        # any sequences that you may have started during a previous call to get_output.
        if self.active_sequence is not None and not self.active_sequence.done:
            controls = self.active_sequence.tick(packet)
            if controls is not None:
                return controls


        # player car inputs
        player = packet.game_cars[self.index]
        car_location = Vec3(player.physics.location)
        car_velocity = Vec3(player.physics.velocity)
        car_ang_velocity = Vec3(player.physics.angular_velocity)

        inputs.extend([self.team_factor * car_location.x / self.max_region.x, self.team_factor * car_location.y / self.max_region.y, car_location.z / self.max_region.z])
        inputs.extend([self.team_factor * car_velocity.x / self.max_velocity.x, self.team_factor * car_velocity.y / self.max_velocity.y, car_velocity.z / self.max_velocity.z])
        inputs.extend([car_ang_velocity.x / self.max_ang_velocity.x, car_ang_velocity.y / self.max_ang_velocity.y, car_ang_velocity.z / self.max_ang_velocity.z])
        inputs.extend([player.boost, player.is_super_sonic, player.jumped or player.double_jumped])

        # opponent car inputs
        opp = packet.game_cars[len(packet.game_cars) - self.index - 1]
        car_location = Vec3(opp.physics.location)
        car_velocity = Vec3(opp.physics.velocity)
        car_ang_velocity = Vec3(opp.physics.angular_velocity)

        inputs.extend([self.team_factor * car_location.x / self.max_region.x, self.team_factor * car_location.y / self.max_region.y, car_location.z / self.max_region.z])
        inputs.extend([self.team_factor * car_velocity.x / self.max_velocity.x, self.team_factor * car_velocity.y / self.max_velocity.y, car_velocity.z / self.max_velocity.z])
        inputs.extend([car_ang_velocity.x / self.max_ang_velocity.x, car_ang_velocity.y / self.max_ang_velocity.y, car_ang_velocity.z / self.max_ang_velocity.z])
        inputs.extend([opp.boost, opp.is_super_sonic, opp.jumped or opp.double_jumped])


        # ball inputs
        ball_location = Vec3(packet.game_ball.physics.location)
        ball_velocity = Vec3(packet.game_ball.physics.location)
        ball_ang_velocity = Vec3(packet.game_ball.physics.location)

        inputs.extend([self.team_factor * ball_location.x / self.max_region.x, self.team_factor * ball_location.y / self.max_region.y, ball_location.z / self.max_region.z])
        inputs.extend([self.team_factor * ball_velocity.x / self.max_velocity.x, self.team_factor * ball_velocity.y / self.max_velocity.y, ball_velocity.z / self.max_velocity.z])
        inputs.extend([ball_ang_velocity.x / self.max_ang_velocity.x, ball_ang_velocity.y / self.max_ang_velocity.y, ball_ang_velocity.z / self.max_ang_velocity.z])

        # boost inputs (sort boosts by amount, then y, then x)
        boosts = deepcopy(self.boost_pad_tracker.boost_pads)
        boosts.sort(key=attrgetter('is_full_boost', 'location.y', 'location.x'))
        for boost in boosts:
            inputs.append(1 if boost.is_active else 0)


        inputs_np = np.array(inputs)
        outputs = self.model.predict(inputs_np)[0]
        print("outputs: {}".format(outputs))

        controls = SimpleControllerState(
            limit_to_safe_range(outputs[0]),    # steer
            limit_to_safe_range(outputs[1]),    # throttle
            limit_to_safe_range(outputs[4]),    # pitch
            limit_to_safe_range(outputs[5]),    # yaw
            limit_to_safe_range(outputs[6]),    # roll
            1 if outputs[3] > 0 else 0,         # jump
            1 if outputs[2] > 0 else 0,         # boost
            1 if outputs[7] > 0 else 0          # handbrake
        )

        return controls
