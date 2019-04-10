/**
*
* @authors Andrei Novikov (pyclustering@yandex.ru)
* @date 2014-2019
* @copyright GNU Public License
*
* GNU_PUBLIC_LICENSE
*   pyclustering is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   pyclustering is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/


#include "fcm.hpp"

#include "utils/metric.hpp"


using namespace ccore::utils::metric;


namespace ccore {

namespace clst {


const double             fcm::DEFAULT_TOLERANCE                       = 0.001;

const std::size_t        fcm::DEFAULT_ITERMAX                         = 100;

const double             fcm::DEFAULT_HYPER_PARAMETER                 = 2.0;


fcm::fcm(const dataset & p_initial_centers, const double p_m, const double p_tolerance, const std::size_t p_itermax) :
    m_tolerance(p_tolerance),
    m_itermax(p_itermax),
    m_initial_centers(p_initial_centers),
    m_degree(2.0 / (p_m - 1.0))
{ }


void fcm::process(const dataset & p_data, cluster_data & p_result) {
    m_ptr_data = &p_data;
    m_ptr_result = (fcm_data *) &p_result;

    m_ptr_result->membership().resize(m_ptr_data->size(), point(m_ptr_data->at(0).size(), 0.0));
    m_ptr_result->centers().assign(m_initial_centers.begin(), m_initial_centers.end());

    double current_change = std::numeric_limits<double>::max();

    for(std::size_t iteration = 0; iteration < m_itermax && current_change > m_tolerance; iteration++) {
        update_membership();
        current_change = update_centers();
    }

    extract_clusters(m_ptr_result->clusters());
}


void fcm::verify(void) const {
    if (m_ptr_data->at(0).size() != m_initial_centers[0].size()) {
        throw std::invalid_argument("Dimension of the input data and dimension of the initial cluster centers must be the same.");
    }
}


double fcm::update_centers(void) {
    const std::size_t dimensions = m_ptr_data->at(0).size();
    double change = 0.0;

    for (std::size_t i = 0; i < m_ptr_result->centers().size(); i++) {
        std::vector<double> dividend(m_ptr_data->at(0).size(), 0.0);
        std::vector<double> divider(m_ptr_data->at(0).size(), 0.0);
        for (std::size_t j = 0; j < m_ptr_data->size(); j++) {
            for (std::size_t dimension = 0; dimension < dimensions; dimension++) {
                dividend[dimension] += m_ptr_data->at(j).at(dimension) * m_ptr_result->membership()[i][j];
                divider[dimension] += m_ptr_result->membership()[i][j];
            }
        }

        point update_center(dimensions, 0.0);
        for (std::size_t dimension = 0; dimension < dimensions; dimension++) {
            update_center[dimension] = dividend[dimension] / divider[dimension];
        }

        change = std::max(change, euclidean_distance(update_center, m_ptr_result->centers().at(i)));
        m_ptr_result->centers().at(i) = std::move(update_center);
    }

    return change;
}


void fcm::update_membership(void) {
    const std::size_t data_size = m_ptr_result->membership().size();
    const std::size_t center_amount = m_ptr_result->centers().size();

    for (std::size_t i = 0; i < data_size; i++) {
        std::vector<double> differences(center_amount, 0.0);
        for (std::size_t j = 0; j < center_amount; j++) {
            differences[j] = euclidean_distance(m_ptr_data->at(i), m_ptr_result->centers().at(j));
        }

        for (std::size_t j = 0; j < center_amount; i++) {
            double divider = 0.0;
            for (std::size_t k = 0; k < center_amount; k++) {
                divider += std::pow( differences[j] / differences[k], m_degree );
            }

            m_ptr_result->membership()[i][j] = 1.0 / divider;
        }
    }
}


void fcm::extract_clusters(cluster_sequence & p_clusters) {
    m_ptr_result->clusters() = cluster_sequence(m_ptr_result->centers().size());
    for (std::size_t i = 0; i < m_ptr_data->size(); i++) {
        auto iter = std::max_element(m_ptr_result->membership().at(i).begin(), m_ptr_result->membership().at(i).end());
        std::size_t index_cluster = iter - m_ptr_result->membership().at(i).begin();

        m_ptr_result->clusters().at(index_cluster).push_back(i);
    }
}


}

}