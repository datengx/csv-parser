#include "csv_parser.h"

using std::vector;
using std::string;

namespace csv_parser {
    /** @file
      * Calculates statistics from CSV files
      */

    void CSVStat::init_vectors() {
        /** - Initialize statistics arrays to NAN
         *  - Should be called (by calc()) before calculating statistics
         */
         
        for (size_t i = 0; i < this->subset_col_names.size(); i++) {
            rolling_means.push_back(0);
            rolling_vars.push_back(0);
            mins.push_back(NAN);
            maxes.push_back(NAN);
            n.push_back(0);
        }
    }

    std::vector<long double> CSVStat::get_mean() {
        /** Return current means */
        std::vector<long double> ret;        
        for (size_t i = 0; i < this->subset_col_names.size(); i++) {
            ret.push_back(this->rolling_means[i]);
        }
        return ret;
    }

    std::vector<long double> CSVStat::get_variance() {
        /** Return current variances */
        std::vector<long double> ret;        
        for (size_t i = 0; i < this->subset_col_names.size(); i++) {
            ret.push_back(this->rolling_vars[i]/(this->n[i] - 1));
        }
        return ret;
    }

    std::vector<long double> CSVStat::get_mins() {
        /** Return current variances */
        std::vector<long double> ret;        
        for (size_t i = 0; i < this->subset_col_names.size(); i++) {
            ret.push_back(this->mins[i]);
        }
        return ret;
    }

    std::vector<long double> CSVStat::get_maxes() {
        /** Return current variances */
        std::vector<long double> ret;        
        for (size_t i = 0; i < this->subset_col_names.size(); i++) {
            ret.push_back(this->maxes[i]);
        }
        return ret;
    }

    std::vector< std::map<std::string, int> > CSVStat::get_counts() {
        /** Get counts for each column */
        std::vector< std::map<std::string, int> > ret;        
        for (size_t i = 0; i < this->subset_col_names.size(); i++) {
            ret.push_back(this->counts[i]);
        }
        return ret;
    }

    std::vector< std::map<int, int> > CSVStat::get_dtypes() {
        /** Get data type counts for each column */
        std::vector< std::map<int, int> > ret;        
        for (size_t i = 0; i < this->subset_col_names.size(); i++) {
            ret.push_back(this->dtypes[i]);
        }
        return ret;
    }

    void CSVStat::calc(bool numeric, bool count, bool dtype) {
        /** Go through all records and calculate specified statistics
         *  @param   numeric Calculate all numeric related statistics
         *  @param   count   Create frequency counter for field values
         *  @param   dtype   Calculate data type statistics
         */
        this->init_vectors();
        vector<std::thread> pool;

        // Start threads
        for (size_t i = 0; i < subset_col_names.size(); i++) {
            pool.push_back(std::thread(&CSVStat::calc_col, this, i));
        }

        // Block until done
        for (auto it = pool.begin(); it != pool.end(); ++it) {
            (*it).join();
        }

        this->clear();
    }

    void CSVStat::calc_col(size_t i) {
        /** Calculate statistics for one column
         *  Meant to be executed by one thread/helper for calc()
         */

        std::deque<vector<string>>::iterator current_record = this->records.begin();
        long double x_n;

        if (this->records.back().size() <= 1)
            this->records.back().pop_back();
        
        while (current_record != this->records.end()) {
            this->count((*current_record)[i], i);
            this->dtype((*current_record)[i], i);

            // Numeric Stuff
            try {
                // Using data_type() to check if field is numeric is faster
                // than catching stold() errors
                if (data_type((*current_record)[i]) >= 2) {
                    x_n = std::stold((*current_record)[i]);

                    // This actually calculates mean AND variance
                    this->variance(x_n, i);
                    this->min_max(x_n, i);
                }
            }
            catch (std::out_of_range) {
                // Ignore for now
            }

            // (*current_record)[i].clear();
            ++current_record;
        }
    }

    void CSVStat::dtype(std::string &record, size_t &i) {
        // Given a record update the type counter
        int type = data_type(record);
        
        if (this->dtypes[i].find(type) !=
            this->dtypes[i].end()) {
            // Increment count
            this->dtypes[i][type]++;
        } else {
            // Initialize count
            this->dtypes[i].insert(std::make_pair(type, 1));
        }
    }

    void CSVStat::count(std::string &record, size_t &i) {
        /** Given a record update the frequency counter */
        if (this->counts[i].find(record) !=
            this->counts[i].end()) {
            // Increment count
            this->counts[i][record]++;
        } else {
            // Initialize count
            this->counts[i].insert(std::make_pair(record, 1));
        }
    }

    void CSVStat::min_max(long double &x_n, size_t &i) {
        /** Update current minimum and maximum */
        if (isnan(this->mins[i])) {
            this->mins[i] = x_n;
        } if (isnan(this->maxes[i])) {
            this->maxes[i] = x_n;
        }
        
        if (x_n < this->mins[i]) {
            this->mins[i] = x_n;
        } else if (x_n > this->maxes[i]) {
            this->maxes[i] = x_n;
        } else {
        }
    }

    void CSVStat::variance(long double &x_n, size_t &i) {
        // Given a record update rolling mean and variance for all columns
        // using Welford's Algorithm
        long double * current_rolling_mean = &this->rolling_means[i];
        long double * current_rolling_var = &this->rolling_vars[i];
        float * current_n = &this->n[i];
        long double delta;
        long double delta2;
        
        *current_n = *current_n + 1;
        
        if (*current_n == 1) {
            *current_rolling_mean = x_n;
        } else {
            delta = x_n - *current_rolling_mean;
            *current_rolling_mean += delta/(*current_n);
            delta2 = x_n - *current_rolling_mean;
            *current_rolling_var += delta*delta2;
        }
    }
}