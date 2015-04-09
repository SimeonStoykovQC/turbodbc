/**
 *  @file cursor.cpp
 *  @date 12.12.2014
 *  @author mkoenig
 *  @brief 
 *
 *  $LastChangedDate$
 *  $LastChangedBy$
 *  $LastChangedRevision$
 *
 */

#include <pydbc/cursor.h>
#include <pydbc/make_description.h>

#include <boost/variant/get.hpp>
#include <sqlext.h>
#include <stdexcept>

#include <cstring>

namespace pydbc {

namespace {

	std::shared_ptr<parameter> make_parameter(cpp_odbc::statement const & statement, std::size_t one_based_index)
	{
		auto const description = statement.describe_parameter(one_based_index);
		return std::make_shared<parameter>(statement, one_based_index, 10, make_description(description));
	}

}

cursor::cursor(std::shared_ptr<cpp_odbc::statement const> statement) :
	statement_(statement),
	current_parameter_set_(0)
{
}

cursor::~cursor() = default;

void cursor::prepare(std::string const & sql)
{
	statement_->prepare(sql);
}

void cursor::execute()
{
	statement_->execute_prepared();

	std::size_t const columns = statement_->number_of_columns();
	if (columns != 0) {
		result_ = std::make_shared<result_set>(statement_, 10);
	}
}

void cursor::bind_parameters()
{
	if (statement_->number_of_parameters() != 0) {
		std::size_t const n_parameters = statement_->number_of_parameters();
		for (std::size_t one_based_index = 1; one_based_index <= n_parameters; ++one_based_index) {
			parameters_.push_back(make_parameter(*statement_, one_based_index));
		}
	}
	statement_->set_attribute(SQL_ATTR_PARAMSET_SIZE, current_parameter_set_);
}

void cursor::add_parameter_set(std::vector<nullable_field> const & parameter_set)
{
	for (unsigned int parameter = 0; parameter != parameter_set.size(); ++parameter) {
		parameters_[parameter]->set(current_parameter_set_, *parameter_set[parameter]);
	}
	++current_parameter_set_;
	statement_->set_attribute(SQL_ATTR_PARAMSET_SIZE, current_parameter_set_);
}

std::vector<nullable_field> cursor::fetch_one()
{
	if (result_) {
		return result_->fetch_one();
	} else {
		throw std::runtime_error("No active result set");
	}
}

long cursor::get_rowcount()
{
	return statement_->row_count();
}

std::shared_ptr<cpp_odbc::statement const> cursor::get_statement() const
{
	return statement_;
}

}
